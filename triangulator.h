#pragma once

#define NUM_THREAD 4

#include "types.h"

// STL
#include <array>
#include <vector>
#include <mutex>
#include <memory>
#include <unordered_set>
#include <algorithm>
// Boost
#include <boost/functional/hash.hpp>

// TBB
#include <tbb/concurrent_hash_map.h>
//#include <tbb/task.h>
//#include <tbb/task_scheduler_init.h>

// declare the functions in predicates.c
#include "predicates.h"

// If the number of points in a tetra is less than CUT_OFF_SIZE, do not create new task.
// TODO Make it more reasonable
#ifndef CUT_OFF_SIZE
#	define CUT_OFF_SIZE 0
#endif

#include "job_queue.h"

//#define DEBUG

using namespace std;

class triangulator {
public:
	int get_num_jobs() const {
		return job_queue_.get_num_jobs();
	}
	// Triangulate the points
	triangulator(std::vector<xyz> const& xyzs, int num_thread) : job_queue_(num_thread) {
		//size_t n = xyzs.size();
           int n = xyzs.size();
	    // Get hull tetra
	    std::array<xyz, 4> hull_xyzs = get_hull_xyzs(xyzs);
	    tetra hull_tetra = {n, n + 1, n + 2, n + 3};
	    // Init point_datas_
	    point_datas_.reset(new point_data[n + 4]);
	    for (size_t i = 0; i < n; ++i) {
		auto&& point_data = point_datas_[i];
		point_data.pos = xyzs[i];
	    }
	    for (size_t i = 0; i < 4; ++i) {
		auto&& point_data = point_datas_[n + i];
		point_data.pos = hull_xyzs[i];
	    }
	    // Prepare points_in_tetra
            tetra_data t_data;
	    t_data.pts_intetra.reserve(n);
	    for (point_k i = 0; i < n; ++i) t_data.pts_intetra.push_back(i);

            for (size_t i = 0; i < 4; i++) {
                for (size_t j = 0; j < 4; j++)
                    t_data.neighbor[i][j] = -1;
            }
        
            // init tetra_to_points_ only have one single tetra
	    tetra_to_points_.insert(tetra_map::value_type(hull_tetra, std::make_shared<tetra_data>(t_data)));
        
            // init to use predicate.c
            exactinit();

	    // Spawn task
	    run_root_task(hull_tetra);
	}

	// Return triangulation result.
	std::vector<tetra> triangulate() {
		std::vector<tetra> ret;
		ret.reserve(tetra_to_points_.size());
		for (auto&& tp : tetra_to_points_) {
			ret.push_back(tp.first);
		}
		return ret;
	}

private:

	// *** POINT STUFF ***

	// mutex struct, each point should have one mutex and one lock, lock aquires mutex
	typedef std::mutex point_mutex;


	struct point_data {
		xyz pos;
		point_mutex p_mutex;	// Note that we don't need point_lock here
		//std::unordered_set<tetra, tetra_hash> neighbor_tetras;	// Is it better to store only 3 points instead of the whole tetra?
	};


	// *** TETRA STUFF ***

	// tetra_data which is the list of points inside a tetrahedron, used as data of hashmap 
	//typedef std::vector<point_k> tetra_data;
	typedef struct {
            std::vector<point_k> pts_intetra;    
            // four neighbors of this tetra, set to the vertex index to -1 if this neighbor is null
            tetra neighbor[4];
        } tetra_data;


	// A concurrent hash table that maps tetra to tetra_data.
	typedef tbb::concurrent_hash_map<tetra, std::shared_ptr<tetra_data>, tetra_hash> tetra_map;

	// *** TBB TASK ***

//	tbb::empty_task* root_task_;

	job_queue job_queue_;

	void run_root_task(tetra t) {
		job_queue_.push_job(triangulation_task(this, t));
		job_queue_.run_jobs();
	}

	void create_new_task(tetra t) {
		job_queue_.push_job(triangulation_task(this, t));
	}

	class triangulation_task {
	public:
		triangulation_task(triangulator* thiz, tetra t) : 
			thiz_(thiz), tetra_(t)
		{
		}
		void operator()() {
			// lock the points
                    if (!try_lock_tetra_points()) {
			thiz_->create_new_task(tetra_);
			lock_fail();
			return;
		    }
			// get tetra's points
		    tetra_map::accessor ac;
		    if (!thiz_->tetra_to_points_.find(ac, tetra_)) {
			unlock_tetra_points();
			return;
		    }
		    // triangulate
		    auto tdata = ac->second;
                    ac.release();
		    triangulate_parallel(tdata->pts_intetra);
                    unlock_tetra_points();
		    return;

		}
	private:
      		// Triangulate with parallel
		void triangulate_parallel(std::vector<point_k>& points_in_tetra) {
                    std::vector<point_mutex*> mutexs;
                    std::unordered_set<tetra, tetra_hash> local_tetras;
                    std::unordered_set<tetra, tetra_hash> boundary_tetras;
                    std::unordered_set<point_k> pts_locked;
                    point_k pt_to_insert = points_in_tetra[0];
                    for (int i = 0; i < 4; i++) pts_locked.insert(tetra_[i]);
                    int test = get_local_tetras(mutexs, local_tetras, pts_locked, boundary_tetras, pt_to_insert, tetra_);
                    if(test == -1) {
                        //unlock mutexs, new task, and return
                        for(auto&& m : mutexs ) {
			    m->unlock();
			}
			thiz_->create_new_task(tetra_);
			lock_fail();
			return;
                    }
                    //cout<<"ddd"<<endl;
                    // locked and local_tetras find, begin to process
		    std::unordered_set<triangle, triangle_hash> local_triangles;
                    get_local_triangles(local_triangles, local_tetras);
                    std::vector<tetra> new_tetras;
                    std::vector<tetra_data> tdata_list;
                    for(auto&& tri_to_connect : local_triangles) {
                        tetra t = {tri_to_connect[0], tri_to_connect[1], tri_to_connect[2], pt_to_insert};
                        std::sort(t.begin(), t.end());
                        new_tetras.push_back(t);
                        tetra_data t_data_tmp;
                        for (int i_t = 0; i_t < 4; i_t++) {
                            for (int i_v = 0; i_v < 4; i_v++)
                                t_data_tmp.neighbor[i_t][i_v] = -1;
                        }
                        tdata_list.push_back(t_data_tmp);
                    }



                    // redistribute points to new_tetras and calculate neighbors for new_tetras
                    // distribute tetra_'s remainning points to new_tetras
                    if(points_in_tetra.size() > 1) {
		        for (std::vector<point_k>::iterator it = points_in_tetra.begin()+1 ; it != points_in_tetra.end(); ++it) {
                            for (int itetra = 0; itetra < new_tetras.size(); itetra++) {
                                auto&& t = new_tetras[itetra];
                                if( intetra(thiz_->point_datas_[t[0]].pos.data(), 
					thiz_->point_datas_[t[1]].pos.data(),
					thiz_->point_datas_[t[2]].pos.data(),
					thiz_->point_datas_[t[3]].pos.data(),
					thiz_->point_datas_[*it].pos.data() ) > 0 ) {
                                    tdata_list[itetra].pts_intetra.push_back(*it);
			    	    break; 
			        }
			    }
			}
		    }
                    // delete tetra_ from local_tetra
                    local_tetras.erase(tetra_);
                    // now distibute the points of other tetras to new tetras
		    for (auto&& old_tetra : local_tetras ) {
			tetra_map::accessor a;
			thiz_->tetra_to_points_.find(a,old_tetra);
			auto t_data_tmp = a->second;
			a.release();
			for (std::vector<point_k>::iterator it = t_data_tmp->pts_intetra.begin() ; it != t_data_tmp->pts_intetra.end(); ++it) { 
                            for (int itetra=0; itetra < new_tetras.size(); itetra++) {
                                auto&& t = new_tetras[itetra];
				if( intetra(thiz_->point_datas_[t[0]].pos.data(), 
				        thiz_->point_datas_[t[1]].pos.data(),
				        thiz_->point_datas_[t[2]].pos.data(),
				        thiz_->point_datas_[t[3]].pos.data(),
				        thiz_->point_datas_[*it].pos.data() ) > 0 ) {
				        tdata_list[itetra].pts_intetra.push_back(*it);
				    break; 
				}
			    }
			}
                        // local_tetras has been deleted from tetra hashmap
			thiz_->tetra_to_points_.erase(old_tetra);
		    }
                    thiz_->tetra_to_points_.erase(tetra_);
                    // insert tetra_ back
                    local_tetras.insert(tetra_);

                    // update new tetras' neighbors
                    for (int itetra = 0; itetra < new_tetras.size(); itetra++) {
                        //cout<<"new tetra "<<new_tetras[itetra][0]<<" "<<new_tetras[itetra][1]<<" "<<new_tetras[itetra][2]<<" "<<new_tetras[itetra][3]<<endl;
                        // compare with new_tetras
                        for (int jtetra = 0; jtetra < new_tetras.size(); jtetra++) {
                            if(itetra == jtetra)
                                continue;
                            if (neighbor_tetra(new_tetras[itetra], new_tetras[jtetra]) == 3) {
                                // update tdata_list[itetra].neighbor
                                for(auto&& nei : tdata_list[itetra].neighbor) {
                                    if(nei[0] == -1) {
                                        nei[0] = new_tetras[jtetra][0];
                                        nei[1] = new_tetras[jtetra][1];
                                        nei[2] = new_tetras[jtetra][2];
                                        nei[3] = new_tetras[jtetra][3];
                                        break;    
                                    }   
                                }
                            }
                        }
                        // compare with boundary tetras
                        for (auto&& b_tetra : boundary_tetras) {
                           if (neighbor_tetra(new_tetras[itetra], b_tetra) == 3) {
                                // update tdata_list[itetra].neighbor
                                for(auto&& nei : tdata_list[itetra].neighbor) {
                                    if(nei[0] == -1) {
                                        nei[0] = b_tetra[0];
                                        nei[1] = b_tetra[1];
                                        nei[2] = b_tetra[2];
                                        nei[3] = b_tetra[3];
                                        break;    
                                    }   
                                }
                            } 
                        }
                    }

                    // delete boundary tetras whose neighbor contain local_tetras, and if new_tetras are its neighbor add to it 
                    for( auto&& b_tetra : boundary_tetras) {
                        tetra_map::accessor ac;
                        thiz_->tetra_to_points_.find(ac, b_tetra);
                        // test with try get t_data and release ac;
                        auto&& t_data = ac->second;
                        ac.release();
                        for(int neighbor_i = 0; neighbor_i < 4; neighbor_i++) {
                            for(auto&& l_tetra : local_tetras) {
                                int flag_eq = 1;
                                for(int v_i = 0; v_i < 4; v_i++) {
                                    if(t_data->neighbor[neighbor_i][v_i] != l_tetra[v_i]) {
                                        flag_eq = 0;
                                        break;
                                    }
                                }
                                if(flag_eq == 0)
                                    continue;
                                // find com tri of this local_tetra and b_tetra
                                std::vector<point_k> com_vs;
                                for(int i=0; i<4; i++) {
                                    for(int j=0; j<4; j++) {
                                        if(b_tetra[i] == l_tetra[j]){
                                            com_vs.push_back(b_tetra[i]);
                                            break;
                                        }
                                    }
                                }
                                com_vs.push_back(pt_to_insert);
                                std::sort(com_vs.begin(),com_vs.end());
                                for(int i=0; i<4; i++) {
                                    t_data->neighbor[neighbor_i][i] = com_vs[i];
                                }
                                break;
                            }
                        }
                    }

                    // delete local_tetras from tetra hashmap, add new_tetras to hashmap and create new task
                    for (int itetra = 0; itetra < new_tetras.size(); itetra++) {
                        thiz_->tetra_to_points_.insert(tetra_map::value_type(new_tetras[itetra], make_shared<tetra_data>(tdata_list[itetra])));
                        if(tdata_list[itetra].pts_intetra.size() > 0) {
                            thiz_->create_new_task(new_tetras[itetra]);
                        }
                    }

                    for(auto&& m : mutexs) {
                        m->unlock();
                    }

                }
        

                // return 1 if lock success and local_tetras calculated
                int get_local_tetras( std::vector<point_mutex*>& mutexs,
				    std::unordered_set<tetra, tetra_hash>& local_tetras,
                                    std::unordered_set<point_k>& pts_locked,
				    std::unordered_set<tetra, tetra_hash>& boundary_tetras,
                                    point_k pt_to_insert, tetra curr_tetra) {
                    local_tetras.insert(curr_tetra);
                    tetra_map::accessor ac;
                    thiz_->tetra_to_points_.find(ac, curr_tetra);
                    auto t_data = ac->second;
                    ac.release();
                    for(auto&& t : t_data->neighbor) { 
                        //if in local_teras
                        //continue
                        if(local_tetras.find(t) != local_tetras.end())
                            continue;
                        if(t[0] == -1)
                            continue;
             	        if( insphere_with_adjust(thiz_->point_datas_[t[0]].pos.data(), 
		            thiz_->point_datas_[t[1]].pos.data(),
		            thiz_->point_datas_[t[2]].pos.data(),
		            thiz_->point_datas_[t[3]].pos.data(),
		            thiz_->point_datas_[pt_to_insert].pos.data() ) > 0 ) {
                            // try lock the vertex not locked yet  
                            for (auto&& v : t) {
                                //find the one not in pts_locked to lock
                                if(pts_locked.find(v) == pts_locked.end()) {
                                    //try lock
        	                    auto&& m = thiz_->point_datas_[v].p_mutex;
                                    //if success lock, add pts_locked, add mutexs and call recusive
			            if(m.try_lock()) {
			                mutexs.push_back(&m);
                                        pts_locked.insert(v);
                                    }
			            else {
                             //           cout<<"try lock fail"<<endl;
                                        return -1;
                                    }
                                    break;
                                } 
                            }
                            // vertex locked, call recursive
                            int test = get_local_tetras(mutexs, local_tetras, pts_locked, boundary_tetras, pt_to_insert, t);
                            if (test == -1)
                                return -1; 
                        } 
                        else {
                            boundary_tetras.insert(t);
                            //continue;
                        }
                    }
                    return 1;
                } 



                void get_local_triangles (std::unordered_set<triangle, triangle_hash>& local_triangles , std::unordered_set<tetra, tetra_hash>& local_tetras) {
                    for(auto&& local_t : local_tetras) {
            	        // insert to local triangles
                        //cout<<"local tetra "<<local_t[0]<<" "<<local_t[1]<<" "<<local_t[2]<<" "<<local_t[3]<<endl;
			triangle tri_insert1 = {local_t[0],local_t[1],local_t[2]};
			triangle tri_insert2 = {local_t[0],local_t[2],local_t[3]};
			triangle tri_insert3 = {local_t[1],local_t[2],local_t[3]};
			triangle tri_insert4 = {local_t[0],local_t[1],local_t[3]};
			
                        if( local_triangles.find(tri_insert1) == local_triangles.end() )
			    local_triangles.insert(tri_insert1);
			else
			    local_triangles.erase(tri_insert1);
			
                        if( local_triangles.find(tri_insert2) == local_triangles.end() )
			    local_triangles.insert(tri_insert2);
			else
			    local_triangles.erase(tri_insert2);
                        
                        if( local_triangles.find(tri_insert3) == local_triangles.end() )
			    local_triangles.insert(tri_insert3);
			else
			    local_triangles.erase(tri_insert3);

                        if( local_triangles.find(tri_insert4) == local_triangles.end() )
			    local_triangles.insert(tri_insert4);
			else
			    local_triangles.erase(tri_insert4);		
                    }
                }

                int neighbor_tetra(tetra tetra_a, tetra tetra_b) {
                    int count = 0;
                    for (int i = 0; i < 4; i++) {
                        for (int j = 0; j < 4; j++) {
                            if(tetra_a[i] == tetra_b[j]) {
                                count++;
                                break;
                            }
                        }
                    }
                    return count;
                }

            
		

		      
		// Try to lock the points of the tetra. Return true iff all points are locked.
		bool try_lock_tetra_points() {
			return -1 == std::try_lock(
					thiz_->point_datas_[tetra_[0]].p_mutex, 
					thiz_->point_datas_[tetra_[1]].p_mutex, 
					thiz_->point_datas_[tetra_[2]].p_mutex, 
					thiz_->point_datas_[tetra_[3]].p_mutex);
		}
		void unlock_tetra_points() {
			for (size_t i = 0; i < 4; ++i) {
				thiz_->point_datas_[tetra_[i]].p_mutex.unlock();
			}
		}
		void lock_fail() {
//			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		triangulator* thiz_;
		tetra tetra_;
	};

public:
	// Return 4 points which can form a tetra containing all points.
    // Presumptin:
    //  all points are generated randomly from [0, 1)
	static std::array<xyz, 4> get_hull_xyzs(std::vector<xyz> const& points) {
        // TODO the tetra seems large enough, should we test?
        xyz vs[4] = {
            {  0, 0, 10 },
            { 10, 0, 0 },
            {  0,10, 0 },
            {  -5, -5,  -5 }
        };

        std::array<xyz, 4> ret =  {
            vs[0], vs[1], vs[2], vs[3]
        };
		return ret;
	}

	// All points including 4 hull points.
	std::unique_ptr<point_data[]> point_datas_;
	// A map containing all valid tetras and the points in them.
	tetra_map tetra_to_points_;

    //std::vector<tetras> vecTetras_;

};

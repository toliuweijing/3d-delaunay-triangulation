

#if __linux__ && defined(__INTEL_COMPILER)
#	define __sync_fetch_and_add(ptr,addend) _InterlockedExchangeAdd(const_cast<void*>(reinterpret_cast<volatile void*>(ptr)), addend)
#endif


#include <cstdlib>
//#include <stdlib.h>
#include <chrono>

int num_points; 
#include "predicates.h"
#include "types.h"
#include "triangulator.h"
// ------------- For drawing ---------------
#define OPENGL
#ifdef OPENGL
#include <GL/glew.h>
#include <GL/glut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "common_shader_utils.h"
#include "glApp.h"
#endif

// ----------------- correctness -----------
//#define CHECK_CORRECTNESS
//
// ------------- my spatial sort -------
#define SPATIAL_SORT
#ifdef SPATIAL_SORT
#include "spatialsort.h"
#endif

using namespace std; 

// Generate points in [0, 1) * [0, 1) * [0, 1) ramdomly.
inline std::vector<xyz> generate_xyzs(size_t num) {
	srand(time(0));
    std::vector<xyz> ret;
	double d = min(0.1, pow(1.0 / num, 0.66) * 5);
    for (int i = 0; i < num; i++)
    {
		for (;;) {
			xyz p = {rand() * 1.0 / RAND_MAX, rand() * 1.0 / RAND_MAX, rand() * 1.0 / RAND_MAX};
			p[0] = p[0] * 2 - 1;
			p[1] = p[1] * 2 - 1;
			p[2] = p[2] * 2 - 1;
			if (p[0] * p[0] + p[1] * p[1] + p[2] * p[2] > 1) continue;

			xyz p2 = {
				0.01 + (p[0] + 1) / 2,
				0.01 + (p[1] + 1) / 2,
				0.01 + (p[2] + 1) / 2
			};

			bool ok = true;
/*
			for (auto&& p3 : ret) {
				xyz p4 = {p2[0] - p3[0], p2[1] - p3[1], p2[2] - p3[2]};
				if (p4[0] * p4[0] + p4[1] * p4[1] + p4[2] * p4[2] < d * d) {
					ok = false;
					break;
				}
			}
*/
			if (!ok) continue;
			ret.push_back(p2);
			break;
		}
    }
    spatial_sort(ret);
    cout << "spatial sorted" << endl;

	return ret;
}

std::vector<xyz> xyzs;
std::vector<tetra> tetras;
triangulator* g_triangulator;

// Output the tetras. Maybe write to file or draw them?
inline void output_tetras(std::vector<tetra> const& tetras) {
    for (auto&& t : tetras)
    {
        cout << t[0] << t[1] << t[2] << t[3] << endl; 
    }
}


#ifdef OPENGL
std::vector<gl::vertex> draw(double time)
{
    using namespace gl;
    std::vector<vertex> vertices;

    int n = 0;
    for (auto&& t : tetras)
    {
        bool skip = false;
        for (auto&& key : t)
        {
            if(key >= num_points)
                skip = true;
        }
        if(skip)
            continue;

        
        //std::cout << ++n << std::endl;
        std::array<vec3, 4> tetra_points;

		for (int i = 0; i < 4; ++i) {
			for (int j = i + 1; j < 4; ++j) {
				if (t[i] == t[j]) {
					cout << "error!" << endl;
				}
			}
		}
        
        tetra_points[0] = vec3(xyzs[t[0]][0],xyzs[t[0]][1],xyzs[t[0]][2]);
        tetra_points[1] = vec3(xyzs[t[1]][0],xyzs[t[1]][1],xyzs[t[1]][2]);
        tetra_points[2] = vec3(xyzs[t[2]][0],xyzs[t[2]][1],xyzs[t[2]][2]);
        tetra_points[3] = vec3(xyzs[t[3]][0],xyzs[t[3]][1],xyzs[t[3]][2]);

        auto tetra_points_to_array = [](vec3 v) -> std::array<REAL, 3> {
            std::array<REAL, 3> ret;
            ret[0] = v[0];
            ret[1] = v[1];
            ret[2] = v[2];
            return ret;
        };

        auto p0 = tetra_points_to_array(tetra_points[0]);
        auto p1 = tetra_points_to_array(tetra_points[1]);
        auto p2 = tetra_points_to_array(tetra_points[2]);
        auto p3 = tetra_points_to_array(tetra_points[3]);


        if (orient3dfast(p0.data(), p1.data(), p2.data(), p3.data())< 0) {
            swap(tetra_points[0], tetra_points[1]);
        }


        auto s3 = create_tetra(tetra_points);
        glm::mat4 m(1.0f);
        m = glm::translate(m, vec3(-0.5, -0.5, -0.5));
		transform_shape(s3, m);
		auto pc = (s3[0].coord + s3[1].coord + s3[2].coord + s3[3].coord) * 0.3f * (-(float)cos(time / 3) + 1);
        m = glm::translate(glm::mat4(1.0f), pc);
		transform_shape(s3, m);
        vertices.insert(vertices.end(), s3.begin(), s3.end());
    }


    return vertices;
}
#endif


void check_correctness(std::vector<tetra>& tetras)
{
    cout << "Checking correctness ..."; 
    for (int i = 0; i < tetras.size(); i++)
    {
        tetra t1 = tetras[i];
        if (t1[0] >= xyzs.size() |
            t1[1] >= xyzs.size() |
            t1[2] >= xyzs.size() |
            t1[3] >= xyzs.size()) 
            continue;


        for (int j = 0; j < tetras.size(); j++)
        {
            tetra t2 = tetras[j];

            if (i == j)
                continue;

            if (t2[0] >= xyzs.size() |
                t2[1] >= xyzs.size() |
                t2[2] >= xyzs.size() |
                t2[3] >= xyzs.size()) 
                continue;

            for (int k = 0; k < 4; k++)
            {
                assert(intetra( xyzs[t1[0]].data(),
                                xyzs[t1[1]].data(),
                                xyzs[t1[2]].data(),
                                xyzs[t1[3]].data(),
                                xyzs[t2[k]].data()) != 1);
                assert(insphere( xyzs[t1[0]].data(),
                                xyzs[t1[1]].data(),
                                xyzs[t1[2]].data(),
                                xyzs[t1[3]].data(),
                                xyzs[t2[k]].data()) != 1);
            }
        }
    }
    cout << " passed" << endl;
}


void CheckParams(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "bad parameters: ./delaunay [num_points] [0/1:display tetras]\n");
        exit(1);
    }
}

int main(int argc, char *argv[])
{
    CheckParams(argc, argv);

	num_points = atoi(argv[1]);
	cout << "Number of points: " << num_points << endl;
	cout << "Generating points..." << endl;
    xyzs = generate_xyzs(num_points);
    int thread_numbers[] = {1, 2, 3, 4};
	for (int i = 0; i < sizeof(thread_numbers) / sizeof(thread_numbers[0]); ++i) {
		int num_thread = thread_numbers[i];
		int num_jobs = 0;
		int num_repeat = 1;
		cout << "Number of threads: " << num_thread << endl;
		cout << "Triangulating..." << endl;
		auto start_time = chrono::steady_clock::now();
		for (int j = 0; j < num_repeat; ++j) {
			triangulator g_triangulator(xyzs, num_thread);
			tetras = g_triangulator.triangulate();
			num_jobs += g_triangulator.get_num_jobs();
		}
		auto end_time = chrono::steady_clock::now();
		double time = 0.001 * chrono::duration_cast<chrono::milliseconds>
			(end_time - start_time).count();
		cout << "Finished." << endl;
		time /= num_repeat;
		cout << "Execution Time: " << time << endl;
		cout << "Number of points per second: " << num_points / time << endl;
		cout << "Number of jobs per second: " << num_jobs / time / num_repeat << endl;
	}
    //cout << "tetras.size = " <<  tetras.size() << endl;


#ifdef CHECK_CORRECTNESS
    check_correctness(tetras);
#endif



#ifdef OPENGL
    if (atoi(argv[2]))
    gl::gl_main(argc, argv, 800, 600, std::function<vector<gl::vertex> (double)>(draw));
#endif
}



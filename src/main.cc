#include <math.h>
#include <unistd.h>

#include <chrono>
#include <unordered_set>
#include <filesystem>
#include <fstream>

#include "dancing_on_zdd.h"
#include "batch_processor.h"
#include "dp_manager.h"

using namespace std;
namespace fs = std::filesystem;
/**
 * main function
 */

// extern uint64_t num_search_tree_nodes;
// extern uint64_t search_tree_depth;
// extern uint64_t search_tree_max_depth;
// extern uint64_t num_solutions;
// extern uint64_t ZddWithLinks::num_updates;
// extern uint64_t num_inactive_updates;

// 示例: 在build/src/目录下运行: ./d3x -d ../../data -o ../../output/zdd_results.txt
int main(int argc, char** argv) {
    int opt;
    string zdd_file_name;
    string input_directory;
    string output_file_path = "../../output/zdd_results.csv";
    bool batch_mode = false;
    
    while ((opt = getopt(argc, argv, "z:d:o:h")) != -1) {
        switch (opt) {
            case 'z':
                zdd_file_name = optarg;
                break;
            case 'd':
                input_directory = optarg;
                batch_mode = true;
                break;
            case 'o':
                output_file_path = optarg;
                break;
            case 'h':
                show_help_and_exit();
                break;
            default:
                show_help_and_exit();
        }
    }

    if (batch_mode && !input_directory.empty()) {
        // 批量处理模式
        cout << "=== ZDD Batch Processing ===" << endl;
        cout << "Input directory: " << input_directory << endl;
        cout << "Output file: " << output_file_path << endl;
        cout << endl;
        
        // process_directory(input_directory, output_file);
         // 检查输入目录是否存在
        if (!fs::exists(input_directory) || !fs::is_directory(input_directory)) {
            cerr << "Error: Input directory does not exist or is not a directory: " 
                << input_directory << endl;
            exit(1);
        }

        // 创建输出文件
        ofstream output_file(output_file_path);
        if (!output_file.is_open()) {
            cerr << "Error: Cannot create output file: " << output_file_path << endl;
            exit(1);
        }

        for (const auto& entry : fs::directory_iterator(input_directory)) {
            if (entry.is_regular_file()) {
                
            }
        }

        
    } else if (!zdd_file_name.empty()) {
        // 单文件处理模式（保持原有功能）
        int num_var = get_num_vars_from_zdd_file(zdd_file_name);
        ZddWithLinks zdd_with_links(num_var, false);
        zdd_with_links.load_zdd_from_file(zdd_file_name);
        
        if (zdd_with_links.sanity()) {
            fprintf(stderr, "initial zdd is invalid\n");
            exit(1);
        }
        
        fprintf(stderr, "load files done\n");
        vector<vector<uint16_t>> solution;
        auto start_time = std::chrono::high_resolution_clock::now();
        zdd_with_links.search(solution, 0);
        auto end_time = std::chrono::high_resolution_clock::now();
        
        printf("%llu,%llu,%llu,%.4f\n", ZddWithLinks::num_search_tree_nodes,
               ZddWithLinks::num_solutions, ZddWithLinks::num_updates,
               std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time).count()
            );
                   
    } else {
        show_help_and_exit();
    }

    // if (zdd_file_name.empty()) {
    //     show_help_and_exit();
    // }

    // int num_var = get_num_vars_from_zdd_file(zdd_file_name);

    // ZddWithLinks zdd_with_links(num_var, false);
    // zdd_with_links.load_zdd_from_file(zdd_file_name);
    // if (zdd_with_links.sanity()) {
    //     fprintf(stderr, "initial zdd is invalid\n");
    // }
    // fprintf(stderr, "load files done\n");
    // vector<vector<uint16_t>> solution;
    // auto start_time = std::chrono::system_clock::now();
    // zdd_with_links.search(solution, 0);
    // auto end_time = std::chrono::system_clock::now();
    // printf("num nodes %llu, num solutions %llu, num updates %llu, "
    //        "time: %llu msecs\n", ZddWithLinks::num_search_tree_nodes,
    //        ZddWithLinks::num_solutions, ZddWithLinks::num_updates,
    //        std::chrono::duration_cast<std::chrono::milliseconds>(end_time -
    //                                                              start_time)
    //            .count());

    return 0;
}
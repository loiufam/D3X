#include <math.h>
#include <unistd.h>

#include <chrono>
#include <unordered_set>
#include <filesystem>
#include <fstream>

#include "dancing_on_zdd.h"
#include "MyCSV.h"
#include "dp_manager.h"

const int D3X_TIME_COLUMN_INDEX = 5; 
const string table_file = "../../../Algorithm_DXD/exp_results.csv"; // 结果表格文件
const string output_d3x_results_file = "../../../Algorithm_DXD/results/Main/d3x_results.csv"; // D3X结果输出文件
// const string d3x_result_csv = "../../output/d3x_results.csv";

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
int get_num_vars_from_zdd_file(const string& file_name) {
    ifstream ifs(file_name);
    if (!ifs) {
        cerr << "can't open " << file_name << endl;
        exit(1);
    }
    string line;
    unordered_set<int> vars;
    while (getline(ifs, line)) {
        if (line[0] == '.' || line[0] == '\n' || line.size() == 0) continue;
        istringstream iss(line);
        int nid;
        int var;
        string lo_str;
        int lo_id;
        string hi_str;
        int hi_id;
        iss >> nid;
        iss >> var;
        vars.emplace(var);
    }
    return vars.size();
}

void show_help_and_exit() {
    std::cerr << "Usage:" << endl;
    std::cerr << "  Single file: ./d3x -z zdd_file" << endl;
    std::cerr << "  Batch mode:  ./d3x -d input_directory [-o output_file]" << endl;
    std::cerr << endl;
    std::cerr << "Options:" << endl;
    std::cerr << "  -z file     Process single ZDD file" << endl;
    std::cerr << "  -d dir      Process all ZDD files in directory" << endl;
    std::cerr << "  -o file     Output results file (default: zdd_results.txt)" << endl;
    std::cerr << "  -h          Show this help message" << endl;
    exit(1);
}

// 示例: 在build/src/目录下运行: ./d3x -d ../../data -o ../../output/zdd_results.txt
// 示例: 在build/src/目录下运行: ./d3x -z ../../data/instance1.zdd -m 1
int main(int argc, char** argv) {
    int opt;
    string zdd_file_name;
    string input_directory;
    string output_file_path = "../../output/zdd_results.csv";
    bool batch_mode = false;

    int d3x_mode = 0;  // 0: D3X, 1: D3XZ
    
    while ((opt = getopt(argc, argv, "z:d:o:m:h")) != -1) {
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
            case 'm':
                d3x_mode = atoi(optarg);
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
        cout << "=== D3X Batch Processing ===" << endl;
        cout << "Input directory: " << input_directory << endl;
        cout << "Output file: " << output_d3x_results_file << endl;
        cout << endl;
        
         // 检查输入目录是否存在
        if (!fs::exists(input_directory) || !fs::is_directory(input_directory)) {
            cerr << "Error: Input directory does not exist or is not a directory: " 
                << input_directory << endl;
            exit(1);
        }

        MyCSV experimentCSV; // 结果CSV处理器
        experimentCSV.readCSV(table_file); // 读取结果表格
        int counter = 0; // 计数器

        for (const auto& entry : fs::directory_iterator(input_directory)) {
            if (entry.is_regular_file()) {
                string file_name = entry.path().stem().string();
                if (file_name == ".DS_Store") continue;

                try {

                    int num_var = get_num_vars_from_zdd_file(entry.path().string());
                    ZddWithLinks zdd_with_links(num_var, false);
                    zdd_with_links.load_zdd_from_file(entry.path().string());
        
                    if (zdd_with_links.sanity()) {
                        fprintf(stderr, "initial zdd is invalid\n");
                        exit(1);
                    }
                    cout << "load file: " << file_name << " done" << endl;

                    vector<vector<uint16_t>> solution;
                    auto start_time = std::chrono::high_resolution_clock::now();

                    zdd_with_links.startTimer();
                    zdd_with_links.search(solution, 0);

                    auto end_time = std::chrono::high_resolution_clock::now();
                    double elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time).count();

                    cout << "solved successfully: " << file_name << ", time: " << std::to_string(elapsed_seconds) << "s" << endl;
                    // printf("num_nodes: %llu, sols: %llu, num_updates: %llu, time: %.4fs\n", zdd_with_links.num_search_tree_nodes,
                    //         zdd_with_links.num_solutions, zdd_with_links.num_updates,
                    //         elapsed_seconds
                    // );
                    experimentCSV.updateTime(
                        file_name, 
                        elapsed_seconds,
                        D3X_TIME_COLUMN_INDEX,
                        false
                    );

                } catch (const std::runtime_error& e) {
                    cout << "求解超时" << endl;

                    experimentCSV.updateTime(
                        file_name, 
                        0.0,
                        D3X_TIME_COLUMN_INDEX,
                        true
                    );
                }
                cout << file_name << " 求解完成." << endl;
                cout << endl;
            }

            experimentCSV.writeCSV(output_d3x_results_file);
        }

        cout << "All Done." << endl;
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
        try{
            vector<vector<uint16_t>> solution;
            auto start_time = std::chrono::high_resolution_clock::now();
            zdd_with_links.startTimer();
            if (d3x_mode == 0) {
                zdd_with_links.search(solution, 0);
                cout << "Solutions: " << zdd_with_links.num_solutions << endl;
            } else if (d3x_mode == 1) {
                auto result = zdd_with_links.D3XZ(0);
                cout << "ZDD Nodes in solution: " << ZddWithLinks::countZDDNodes(result, zdd_with_links.T_ZDD, zdd_with_links.F_ZDD) << endl;
                cout << "Solutions: " << ZddWithLinks::countZDDSolutions(result, zdd_with_links.T_ZDD, zdd_with_links.F_ZDD) << endl;
            } else {
                cerr << "Invalid D3X mode: " << d3x_mode << ". Use 0 for D3X, 1 for D3XZ." << endl;
                exit(1);
            }
            auto end_time = std::chrono::high_resolution_clock::now();
            double elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time).count();

            cout << "Time: " << std::to_string(elapsed_seconds) << "s" << endl;
            // printf("Solutions: %llu, Time: %.4f s\n", 
            //        zdd_with_links.num_solutions, std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time).count()
            //     );
        } catch (const std::runtime_error& e) {
            cout << "D3X求解超时" << endl;
        }
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
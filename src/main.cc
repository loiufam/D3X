#include <math.h>
#include <unistd.h>

#include <chrono>
#include <unordered_set>
#include <filesystem>
#include <fstream>

#include "dancing_on_zdd.h"
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
        output_file << "Filename,Nodes,sols,Updates,Time(s),Status" << endl;

        if (!output_file.is_open()) {
            cerr << "Error: Cannot create output file: " << output_file_path << endl;
            exit(1);
        }

        for (const auto& entry : fs::directory_iterator(input_directory)) {
            if (entry.is_regular_file()) {
                string file_name = entry.path().stem().string();
                output_file << file_name << ",";

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
                    zdd_with_links.stopwatch.markStartTime();
                    zdd_with_links.search(solution, 0);
                    auto end_time = std::chrono::high_resolution_clock::now();

                    printf("num_nodes: %llu, sols: %llu, num_updates: %llu, time: %.4fs\n", zdd_with_links.num_search_tree_nodes,
                            zdd_with_links.num_solutions, zdd_with_links.num_updates,
                            std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time).count()
                    );

                    output_file << zdd_with_links.num_search_tree_nodes << ","
                                << zdd_with_links.num_solutions << ","
                                << zdd_with_links.num_updates << ","
                                << std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time).count() << ","
                                << "SUCCESS\n";
                    output_file.flush();

                } catch (const std::runtime_error& e) {
                    output_file  << "-,"
                                << "-,"
                                << "-,"
                                << "-,"
                                << "FAILED\n";
                    output_file.flush();
                }
                cout << file_name << " done." << endl;
                cout << endl;
            }
        }
        output_file.close();
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
        vector<vector<uint16_t>> solution;
        auto start_time = std::chrono::high_resolution_clock::now();
        zdd_with_links.search(solution, 0);
        auto end_time = std::chrono::high_resolution_clock::now();
        
        printf("%llu,%llu,%llu,%.4f\n", zdd_with_links.num_search_tree_nodes,
               zdd_with_links.num_solutions, zdd_with_links.num_updates,
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
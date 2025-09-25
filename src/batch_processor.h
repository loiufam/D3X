#ifndef BATCH_PROCESSOR_H
#define BATCH_PROCESSOR_H

#include <filesystem>
#include <iomanip>
#include <unordered_set>
#include <cstdio>
#include <cstdlib>

#include "dancing_on_zdd.h" 
#include "dp_manager.h" 

using namespace std;

struct ProcessResult {
    string filename;
    int num_vars;
    uint64_t num_nodes;
    uint64_t num_solutions;
    uint64_t num_updates;
    uint64_t time_msecs;
    bool success;
    string error_message;
    
    ProcessResult();
};

// 函数声明
int get_num_vars_from_zdd_file(const string& file_name);
ProcessResult process_single_zdd_file(const string& zdd_file_path);
void write_results_header(ofstream& output_file);
void write_result_line(ofstream& output_file, const ProcessResult& result);
void process_directory(const string& input_dir, const string& output_file_path);
void show_help_and_exit();

#endif // BATCH_PROCESSOR_H
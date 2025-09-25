#include "batch_processor.h"

namespace fs = std::filesystem;

ProcessResult::ProcessResult() : num_vars(0), num_nodes(0), num_solutions(0), 
                                num_updates(0), time_msecs(0), success(false) {}

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

ProcessResult process_single_zdd_file(const string& zdd_file_path) {
    ProcessResult result;
    result.filename = fs::path(zdd_file_path).filename().string();
    
    try {
        // 获取变量数量
        result.num_vars = get_num_vars_from_zdd_file(zdd_file_path);
        
        // 创建ZDD对象并加载文件
        ZddWithLinks zdd_with_links(result.num_vars, false);
        zdd_with_links.load_zdd_from_file(zdd_file_path);
        
        // 检查ZDD有效性
        if (zdd_with_links.sanity()) {
            result.error_message = "initial zdd is invalid";
            return result;
        }
        
        // 执行搜索
        vector<vector<uint16_t>> solution;
        auto start_time = std::chrono::system_clock::now();
        zdd_with_links.search(solution, 0);
        auto end_time = std::chrono::system_clock::now();
        
        // 记录结果
        result.num_nodes = ZddWithLinks::num_search_tree_nodes;
        result.num_solutions = ZddWithLinks::num_solutions;
        result.num_updates = ZddWithLinks::num_updates;
        result.time_msecs = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time).count();
        result.success = true;
        
    } catch (const exception& e) {
        result.error_message = e.what();
    } catch (...) {
        result.error_message = "Unknown error occurred";
    }
    
    return result;
}

void write_results_header(ofstream& output_file) {
    output_file << "==================================================" << endl;
    output_file << "ZDD Batch Processing Results" << endl;
    output_file << "==================================================" << endl;
    output_file << "Processing Time: " << 
        chrono::duration_cast<chrono::seconds>(
            chrono::system_clock::now().time_since_epoch()).count() << endl;
    output_file << endl;
    
    // 写入表头
    output_file << left << setw(25) << "Filename" 
               << right << setw(8) << "Vars"
               << right << setw(12) << "Nodes" 
               << right << setw(12) << "Solutions"
               << right << setw(12) << "Updates"
               << right << setw(10) << "Time(ms)"
               << right << setw(10) << "Status" << endl;
    output_file << string(95, '-') << endl;
}

void write_result_line(ofstream& output_file, const ProcessResult& result) {
    output_file << left << setw(25) << result.filename;
    
    if (result.success) {
        output_file << right << setw(8) << result.num_vars
                   << right << setw(12) << result.num_nodes
                   << right << setw(12) << result.num_solutions
                   << right << setw(12) << result.num_updates
                   << right << setw(10) << result.time_msecs
                   << right << setw(10) << "SUCCESS";
    } else {
        output_file << right << setw(8) << "-"
                   << right << setw(12) << "-"
                   << right << setw(12) << "-"
                   << right << setw(12) << "-"
                   << right << setw(10) << "-"
                   << right << setw(10) << "FAILED";
    }
    output_file << endl;
    
    if (!result.success) {
        output_file << "    Error: " << result.error_message << endl;
    }
}


void process_directory(const string& input_dir, const string& output_file_path) {
    // 检查输入目录是否存在
    if (!fs::exists(input_dir) || !fs::is_directory(input_dir)) {
        cerr << "Error: Input directory does not exist or is not a directory: " 
             << input_dir << endl;
        exit(1);
    }
    
    // 创建输出文件
    ofstream output_file(output_file_path);
    if (!output_file.is_open()) {
        cerr << "Error: Cannot create output file: " << output_file_path << endl;
        exit(1);
    }
    
    // 收集所有ZDD文件
    vector<string> zdd_files;
    for (const auto& entry : fs::directory_iterator(input_dir)) {
        if (entry.is_regular_file()) {
            string extension = entry.path().extension().string();
            // 假设ZDD文件扩展名为.zdd，可以根据实际情况调整
            if (extension == ".zdd" || extension == ".txt") {
                zdd_files.push_back(entry.path().string());
            }
        }
    }
    
    if (zdd_files.empty()) {
        cerr << "Warning: No ZDD files found in directory: " << input_dir << endl;
        output_file << "No ZDD files found in directory: " << input_dir << endl;
        return;
    }
    
    // 写入文件头
    write_results_header(output_file);
    
    cout << "Processing " << zdd_files.size() << " files..." << endl;
    
    vector<ProcessResult> results;
    int processed = 0;
    
    // 处理每个文件
    for (const auto& file_path : zdd_files) {
        processed++;
        cout << "Processing [" << processed << "/" << zdd_files.size() << "]: " 
             << fs::path(file_path).filename().string() << "..." << endl;
        
        ProcessResult result = process_single_zdd_file(file_path);
        results.push_back(result);
        
        // 写入结果到文件
        write_result_line(output_file, result);
        output_file.flush(); // 确保实时写入
        
        // 控制台输出进度
        if (result.success) {
            cout << "  -> SUCCESS: " << result.num_solutions << " solutions, " 
                 << result.time_msecs << " ms" << endl;
        } else {
            cout << "  -> FAILED: " << result.error_message << endl;
        }
    }
    
    cout << "\nBatch processing completed!" << endl;
    cout << "Results saved to: " << output_file_path << endl;
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
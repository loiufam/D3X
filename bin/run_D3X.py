#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import subprocess
import csv
import re
import sys
import argparse
from pathlib import Path
from datetime import datetime

# 配置
D3X_EXECUTABLE = "./d3x"
ZDD_EXP_SET = "../data/batch_2"
RESULTS_FOLDER = "../output"

# 算法配置：(算法名, 额外参数, 时间列, 解数列, ZDD Size列, 输出文件名)
ALGORITHMS = [
    ("D3X",  [],          3, 5, None, "D3X_results_batch2.csv"),
    # ("D3XZ", ["-m", "1"], 4, 6, 7,   "D3XZ_results_batch2.csv"),
]

# CSV 表头
CSV_HEADER = ['Instance', '#cols', '#rows', 'D3X_Time', 'D3XZ_Time',
              'D3X_Sols', 'D3XZ_Sols', 'ZDD_Size']
NUM_COLS = len(CSV_HEADER)


def parse_log_output(output):
    """解析算法输出的日志信息"""
    result = {
        'time': None,
        'solutions': None,
        'zdd_size': None,
        'status': 'success',
        'timeout': False,
    }

    if '超时' in output:
        result['timeout'] = True
        return result

    time_match = re.search(r'Time:\s*([\d.]+)\s*s', output)
    if time_match:
        result['time'] = float(time_match.group(1))

    solutions_match = re.search(r'Solutions:\s*([\d.eE+\-]+)', output)
    if solutions_match:
        result['solutions'] = solutions_match.group(1)

    zdd_size_match = re.search(r'ZDD Nodes in solution:\s*(\d+)', output)
    if zdd_size_match:
        result['zdd_size'] = int(zdd_size_match.group(1))

    if result['solutions'] and float(result['solutions']) == 0:
        result['status'] = 'warning'

    return result


def run_algorithm(algo_name, extra_params, input_file, timeout=600):
    """运行单个算法"""
    cmd = [D3X_EXECUTABLE, "-z", input_file] + extra_params
    print(f"  运行命令: {' '.join(cmd)}")

    try:
        proc = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=timeout,
            encoding='utf-8',
            errors='replace',
        )
        output = proc.stdout + proc.stderr
        print(f"  输出: {output[:200]}...")
        return parse_log_output(output)

    except subprocess.TimeoutExpired:
        print(f"  [超时] 运行超过设定上限 {timeout} 秒。")
        return {'time': None, 'solutions': None, 'zdd_size': None,
                'status': 'timeout', 'timeout': True}
    except Exception as e:
        print(f"  错误: {e}")
        return {'time': None, 'solutions': None, 'zdd_size': None,
                'status': 'error', 'timeout': False}


def get_input_files(folder):
    """获取输入文件夹中的所有文件"""
    if not os.path.exists(folder):
        print(f"错误：输入文件夹不存在: {folder}")
        return []
    return sorted(str(f) for f in Path(folder).rglob('*') if f.is_file())


def read_existing_csv(csv_path):
    """读取已有的CSV文件，返回 {instance_name: [row]} 字典"""
    if not os.path.exists(csv_path):
        return {}

    data = {}
    with open(csv_path, 'r', encoding='utf-8') as f:
        reader = csv.reader(f)
        rows = list(reader)
        for row in rows[1:]:  # 跳过表头
            if row:
                data[row[0]] = row
    return data


def write_csv_results(csv_path, results_data, time_col, sols_col, zdd_col):
    """将结果写入CSV文件"""
    existing_data = read_existing_csv(csv_path)

    for filename, result in results_data.items():
        if filename not in existing_data:
            existing_data[filename] = [''] * NUM_COLS
            existing_data[filename][0] = filename

        row = existing_data[filename]
        # 确保行有足够的列
        while len(row) < NUM_COLS:
            row.append('')

        # 时间
        if result.get('timeout'):
            row[time_col] = 'timeout'
        elif result.get('time') is not None:
            cell = f"{result.get('time'):.4f}"
            if result.get('status') == 'warning':
                cell += ' (WARNING: 0 solutions)'
            row[time_col] = cell

        # 解数量
        if sols_col is not None and result.get('solutions') is not None:
            row[sols_col] = result.get('solutions')
 
        # ZDD Size
        if zdd_col is not None and result.get('zdd_size') is not None:
            row[zdd_col] = str(result.get('zdd_size'))

    with open(csv_path, 'w', encoding='utf-8', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(CSV_HEADER)
        for filename in sorted(existing_data.keys()):
            writer.writerow(existing_data[filename])


def main():
    parser = argparse.ArgumentParser(description='批量运行 D3X/D3XZ 算法测试脚本')
    parser.add_argument('-i', '--input-folder', type=str, default=ZDD_EXP_SET,
                        help=f'输入文件夹路径，默认为 {ZDD_EXP_SET}')
    parser.add_argument('-o', '--output-folder', type=str, default=RESULTS_FOLDER,
                        help=f'结果输出文件夹路径，默认为 {RESULTS_FOLDER}')
    parser.add_argument('-a', '--append', action='store_true',
                        help='追加模式：跳过已存在于CSV中的测例')
    parser.add_argument('-t', '--timeout', type=int, default=600,
                        help='单次求解的超时时间（秒），默认 600')
    args = parser.parse_args()

    input_folder = args.input_folder
    results_folder = args.output_folder

    print("=" * 60)
    print("批量算法测试脚本")
    print("=" * 60)
    print(f"输入文件夹: {input_folder}")
    print(f"结果文件夹: {results_folder}")
    if args.append:
        print("[*] 追加模式已开启：将跳过已有记录的文件")
    print(f"超时时间: {args.timeout} 秒")

    os.makedirs(results_folder, exist_ok=True)

    input_files = get_input_files(input_folder)
    if not input_files:
        print("错误：没有找到输入文件")
        return

    print(f"\n找到 {len(input_files)} 个输入文件\n")

    if not os.path.exists(D3X_EXECUTABLE):
        print(f"错误：可执行文件不存在: {D3X_EXECUTABLE}")
        return

    start_time = datetime.now()

    for algo_name, extra_params, time_col, sols_col, zdd_col, output_file in ALGORITHMS:
        print(f"\n{'=' * 60}")
        print(f"运行算法: {algo_name}")
        print(f"{'=' * 60}")

        results_data = {}
        csv_path = os.path.join(results_folder, output_file)

        # 读取已有结果以实现断点续传跳过逻辑
        existing_data = read_existing_csv(csv_path) if args.append else {}

        for i, input_file in enumerate(input_files, 1):
            filename = os.path.basename(input_file)

            if args.append and filename in existing_data:
                print(f"\n[{i}/{len(input_files)}] (追加模式) 跳过已有文件: {filename}")
                continue

            print(f"\n[{i}/{len(input_files)}] 处理文件: {filename}")

            result = run_algorithm(algo_name, extra_params, input_file, timeout=args.timeout)
            results_data[filename] = result

            # 实时写入结果
            write_csv_results(csv_path, results_data, time_col, sols_col, zdd_col)
            results_data = {}

    elapsed = (datetime.now() - start_time).total_seconds()
    print("\n" + "=" * 60)
    print("所有算法运行完成！")
    print(f"总耗时: {elapsed:.2f} 秒")
    print("=" * 60)


if __name__ == "__main__":
    main()
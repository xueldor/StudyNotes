import os
import sys
import shutil
import subprocess
import threading
import json
from datetime import datetime


def runShell(script_path) :
    try:
        result = subprocess.run(['bash', script_path], check=True, text=True, capture_output=True)
        if result.returncode==0:
            print("Output:", result.stdout)  # 获取脚本的输出
            return True
        else:
            print("ErrorMsg:", result.stderr)
            return False
    except subprocess.CalledProcessError as e:
        print("Error:", e.stderr)  # 获取错误输出
        return False

def runCommand(commandStr) :
    try:
        result = subprocess.run(commandStr, shell=True, text=True, capture_output=True)
        if result.returncode==0:
            print("Output:", result.stdout)  # 获取脚本的输出
            return True
        else:
            print("ErrorMsg:", result.stderr)
            return False
    except subprocess.CalledProcessError as e:
        print("Error:", e.stderr)  # 获取错误输出
        return False

def read_stream(stream):
    for line in iter(stream.readline, b''):
        if line:
            print(line, end='', flush=True)
        else:
            return

        

def runCommandInDir(commandStr, directory_path) :
    print("run command:", commandStr, "at", directory_path)
    try:
        process = subprocess.Popen(commandStr, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, cwd=directory_path)
        stdout_thread = threading.Thread(target=read_stream, args=(process.stdout,))
        stderr_thread = threading.Thread(target=read_stream, args=(process.stderr,))
        stdout_thread.start()
        stderr_thread.start()
        process.wait()

        stdout_thread.join()
        stderr_thread.join()
    except Exception as e:
        print(f"发生错误: {e}")
    finally:
        # 确保进程正确关闭
        if process.stdout:
            process.stdout.close()
        if process.stderr:
            process.stderr.close()

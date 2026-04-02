#!/usr/bin/env python3

#    _____                 _                       
#   / ____|               | |                      
#  | |  __  ___  __ _ _ __| |     __ _ _ __   __ _ 
#  | | |_ |/ _ \/ _` | '__| |    / _` | '_ \ / _` | Clean, Clear and Fast Code
#  | |__| |  __/ (_| | |  | |___| (_| | | | | (_| | https://github.com/kwphil/gearlang
#   \_____|\___|\__,_|_|  |______\__,_|_| |_|\__, |
#                                             __/ |
#                                            |___/ 
#
# Licensed under the MIT License <https://opensource.org/licenses/MIT>.
# SPDX-License-Identifier: MIT
#
# Permission is hereby  granted, free of charge, to any  person obtaining a copy
# of this software and associated  documentation files (the "Software"), to deal
# in the Software  without restriction, including without  limitation the rights
# to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
# copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
# IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
# FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
# AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
# LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

from pathlib import Path
import colorama
from colorama import Fore, Style 
import json
import subprocess

colorama.init()

test_path = Path('tests')
executable = './gear'

fail_count = 0
pass_count = 0
test_count = 0
test_name: str
test_type: str

def print_test(passes: bool, fail_str: str = ""):
    global test_count
    global test_name
    global test_type

    match test_type:
        case 'lexer':
            test_header = f'{Fore.CYAN}LEXER{Style.RESET_ALL}'
        case 'parser':
            test_header = f'{Fore.GREEN}PARSE{Style.RESET_ALL}'

    if passes:
        print(f"{test_header} {test_name} --- {Fore.GREEN}PASSED.{Style.RESET_ALL}", flush=True)
    else:
        print(f"{test_header} {test_name} --- {Fore.RED}FAILED.{Style.RESET_ALL} {fail_str}", flush=True)

def run_test(test_data: Path, test_code: Path):
    global test_count
    global test_type
    global test_name

    test_count += 1

    try:
        with open(test_data, 'r', encoding='utf-8') as json_file:
            data = json.load(json_file)
    except json.JSONDecodeError as err:
        print(f"Test {test_name} --- Error: failed to decode test data. Skipping.")
        print("========= json output ============")
        print(err)
        print("==================================")
        return

    def print_type(color, str):
        return f"{color}{str}{Style.RESET_ALL}"

    match data['type']:
        case 'lexer':
            test_flag = '--dump-tokens'
        case 'parser':
            test_flag = '--dump-ast'
        case _:
            print(f"{print_type(Fore.RED, "INVAL")} {test_name} --- {print_type(Fore.LIGHTBLACK_EX, "SKIPPED.")} Unknown type: {data['type']}")
            return

    test_type = data['type']

    output = subprocess.run([ executable, test_flag, test_code ], capture_output=True)

    match_output(output, data)

def recurse_ast(a, b, key) -> bool:
    global fail_count
    global pass_count

    if type(a) != type(b):
        print_test(False, f"Expected type: {type(b)}, received type {type(a)} in key {key}")
        fail_count+=1
        return False

    if isinstance(a, dict):
        if set(a.keys()) != set(b.keys()):
            print_test(False, f"Expected form for object: {set(b.keys())}, received: {set(a.keys())} in key {key}")
            fail_count+=1
            return False
        return all(recurse_ast(a[k], b[k], k) for k in a)

    elif isinstance(a, list):
        if len(a) != len(b):
            print_test(False, f"Expected array to contain {len(b)} elements, but received {len(a)} in key {key}")
            fail_count+=1
            return False
        return all(recurse_ast(x, y, key) for x, y in zip(a, b))

    else:
        if a != b:
            print_test(False, f"Expected primitive: {b}, received {a} in key {key}")
            fail_count+=1
            return False
        
        return True
    
def ast_match(test_output: json.JSONDecoder, test_data: json.JSONDecoder):
    global fail_count
    global pass_count

    test_match = test_data['match']

    if len(test_output) != len(test_match):
        print_test(False, f"expected length {len(test_match)}, received {len(test_output)}")
        fail_count+=1
        return
    
    index = 0
    for curr in test_match:
        if not recurse_ast(test_output[index], curr, ""): 
            return

        index+=1

    print_test(True)
    pass_count+=1

def lexer_match(test_output: json.JSONDecoder, test_data: json.JSONDecoder):
    global fail_count
    global pass_count

    test_match = test_data['match']

    if len(test_output) != len(test_match):
        print_test(False, f"expected length {len(test_match)}, received {len(test_output)}")
        fail_count+=1
        return
    
    index = 0
    for curr in test_match:
        if test_output[index][0] != curr[0]:
            print_test(False, f"Content wrong on index: {index}")
            fail_count+=1
            return

        if test_output[index][1] != curr[1]:
            print_test(False, f"Line wrong on index: {index}")
            fail_count+=1
            return

        if test_output[index][2] != curr[2]:
            print_test(False, f"Type wrong on index: {index}")
            fail_count+=1
            return

        index+=1

    print_test(True, "")
    pass_count+=1

def match_output(output: subprocess.CompletedProcess, test_data: json.JSONDecoder):
    global fail_count
    global pass_count
    global test_name
    global test_type

    if output.returncode != test_data['return']:
        print_test(False, f"Expected return: {test_data['return']} received: {output.returncode}")
        fail_count+=1
        if output.returncode:
            print("===== Fail Message: =============")
            print(output.stderr.decode())
            print("=================================")
        return
    
    if output.returncode != 0:
        print_test(True, "")
        pass_count += 1
        return

    try:
        data = json.loads(output.stdout)
    except json.JSONDecodeError as err:
        print_test(False, "Malformed output")
        print("===== JSON Decoder ===============")
        print(err)
        print("==================================")
        fail_count += 1
        return

    match test_type:
        case 'lexer':
            return lexer_match(data, test_data)
        case 'parser':
            return ast_match(data, test_data)

for curr_test in test_path.iterdir():
    if not curr_test.is_dir():
        continue

    test_data = curr_test / 'test.json'
    test_code = curr_test / 'test.gear' 

    test_name = curr_test.name

    run_test(test_data, test_code)

print(f"{fail_count} failed, {pass_count} passed out of {test_count}")

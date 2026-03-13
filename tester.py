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

def print_test(passes: bool, fail_str: str):
    global test_count
    global test_name

    if passes:
        print(f"{test_name} --- {Fore.GREEN}PASSED.{Style.RESET_ALL}")
    else:
        print(f"{test_name} --- {Fore.RED}FAILED.{Style.RESET_ALL} {fail_str}")

def run_test(test_data: Path, test_code: Path):
    global test_count

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

    match data['type']:
        case 'lexer':
            test_type = 'lexer'
            test_flag = '--dump-tokens'

    output = subprocess.run([ executable, test_flag, test_code ], capture_output=True)

    match_output(output, test_type, data)

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

def match_output(output: subprocess.CompletedProcess, test_type: str, test_data: json.JSONDecoder):
    global fail_count
    global pass_count
    global test_name

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
    except json.JSONDecodeError:
        print_test(False, "Malformed output")
        return

    match test_type:
        case 'lexer':
            return lexer_match(data, test_data)

for curr_test in test_path.iterdir():
    if not curr_test.is_dir():
        continue

    test_data = curr_test / 'test.json'
    test_code = curr_test / 'test.gear' 

    test_name = curr_test.name

    run_test(test_data, test_code)

print(f"{fail_count} failed, {pass_count} passed out of {test_count}")

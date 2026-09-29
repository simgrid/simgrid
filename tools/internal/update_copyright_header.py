#!/usr/bin/env python3
import sys
import re
import subprocess
import argparse
import os

def get_date_interval(filepath, existing_line):
    start_year = None
    
    match = re.search(r'\(c\) ([\d, -]+)\. The SimGrid Team', existing_line)
    if match:
        for token in re.split(r'[, ]+', match.group(1)):
            if not token: 
                continue
            if '-' in token:
                y = int(token.split('-')[0])
            elif token.isdigit():
                y = int(token)
            else:
                continue
            if start_year is None or y < start_year:
                start_year = y

    git_years = []
    try:
        res = subprocess.run(
            ['git', 'log', '--follow', '--format=%ad', '--date=format:%Y', filepath],
            capture_output=True, text=True, check=True
        )
        git_years = [int(y) for y in res.stdout.splitlines() if y.isdigit()]
    except subprocess.CalledProcessError:
        pass

    if git_years:
        git_start = min(git_years)
        git_end = git_years[0]
        
        start_year = min(start_year, git_start) if start_year else git_start
        end_year = git_end
    else:
        if not start_year:
            return ""
        end_year = start_year

    if start_year == end_year:
        return str(start_year)
    return f"{start_year}-{end_year}"

def main():
    parser = argparse.ArgumentParser(description="Update Copyright headers in-place.")
    parser.add_argument('files', nargs='*', help="Files to process (if empty, uses git grep)")
    args = parser.parse_args()

    files_to_process = args.files
    if not files_to_process:
        try:
            grep_res = subprocess.run(
                ['git', 'grep', '-l', 'The SimGrid Team. All rights reserved'],
                capture_output=True, text=True, check=True
            )
            files_to_process = grep_res.stdout.splitlines()
        except subprocess.CalledProcessError:
            sys.exit("Error: No files provided and git grep found no matches.")

    # Hardcoded strict pattern
    pattern = r'Copyright \(c\) ([\d, -]+)\. The SimGrid Team\. All rights reserved\.'

    for filepath in files_to_process:
        print(f"########## {filepath} ##########")
        
        if not os.path.isfile(filepath):
            print("!!! skip (not a file)")
            continue

        with open(filepath, 'r') as f:
            lines = f.readlines()

        modified = False
        for i, line in enumerate(lines):
            if re.search(pattern, line):
                dates_str = get_date_interval(filepath, line)
                if dates_str:
                    new_line = re.sub(
                        r'\(c\) [\d, -]+\. The SimGrid Team\. All rights reserved\.', 
                        f'(c) {dates_str}. The SimGrid Team. All rights reserved.', 
                        line
                    )
                    if new_line != line:
                        lines[i] = new_line
                        modified = True
                break
        
        if not modified:
            print("Pass: no changes needed or no strict SimGrid Copyright header found.")
            continue

        with open(filepath, 'w') as f:
            f.writelines(lines)

    print("\nAll files processed.\n\n*** DO NOT FORGET TO DOUBLE CHECK CHANGES BEFORE DOING ANY COMMIT! ***\n")

if __name__ == '__main__':
    main()
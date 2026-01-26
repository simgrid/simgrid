#!/usr/bin/env python3
import sys
import os
import glob
import xml.etree.ElementTree as ET

# -----------------------------
# Helpers
# -----------------------------

def extract_leaks(root):
    """Extract leak summary values as integers."""



# -----------------------------
# JUnit generation
# -----------------------------

def generate_junit(valgrind_files, junit_output):
    testsuite = ET.Element("testsuite")
    testsuite.set("name", "ValgrindFiles")
    testsuite.set("tests", "0")
    testsuite.set("failures", "0")

    tests = 0
    failures = 0

    summary_all_tests = {}
    for file in valgrind_files:
        tests += 1

        try:
            tree = ET.parse(file)
        except ET.ParseError:
            print (f"Parse error in {file}. Please remove it if it's currently not ongoing.")
            continue
        root = tree.getroot()
        errors = root.findall("error")
        test = root.findtext("usercomment", default="Test_Unknown test") [len("Test_"):]

        # -------------------------
        # Output the errors on the standard output
        # -------------------------
        report = ""
        shown_header = False
        totals = {}
        for error in errors:
            # Do not report memleaks
            kind = error.findtext("kind")
            if kind in ["Leak_StillReachable",  "Leak_IndirectlyLost", "Leak_PossiblyLost", "Leak_DefinitelyLost"]:
                continue

            what = error.findtext("what")
            if not what:
                what = error.findtext("xwhat/text", default="Unknown Error")
            report += f"Error {kind}: {what}\n"
            for frame in error.findall("stack/frame"):
                dirname = frame.findtext("dir", default="unknown")
                if dirname.startswith("../../"):
                    dirname = dirname[len("../../"):]
                dirname = os.path.normpath(dirname)
                filename = frame.findtext("file", default="unknown")
                line = frame.findtext("line", default="?")
                fn = frame.findtext("fn", default="unknown")
                report += f"  at {dirname}/{filename}:{line} {fn}\n"
            report += "\n"
            
            if kind not in totals:
                totals[kind] = 1
            else:
                totals[kind] += 1

        testcase = ET.SubElement(testsuite, "testcase")
        testcase.set("name", test)
        if len(report) > 0:
            # Textual output
            all_errors = ""
            for key in totals:
                all_errors += f"{totals[key]} {key} "
            summary_all_tests[test] = all_errors

            print (f"== {test}: {all_errors} ==")
            print (f"File {file}\n")
            print(report)

            # junit output
            failures += 1
            failure = ET.SubElement(testcase, "failure")
            failure.set("type", "ValgrindError")
            failure.text = report

        else:
            systemout = ET.SubElement(testcase, "system-out")
            systemout.text = f"No errors in {test}"

    print("\n== Overall summary ==")
    for test in sorted(summary_all_tests):
        print(f"  {test}: {summary_all_tests[test]}")
    testsuite.set("tests", str(tests))
    testsuite.set("failures", str(failures))

    ET.ElementTree(testsuite).write(junit_output, encoding="utf-8", xml_declaration=True)

    print(f"[JUnit] {tests} files processed: {junit_output} (failures: {failures})")


# -----------------------------
# HTML report generation
# -----------------------------

def generate_html(valgrind_files, output_html):
    rows = {} # One chunk of HTML per test name, a row in the summary table
    reports = {} # One chunk of HTML per test name, the details of each callsite
    leaks_per_test = {} # amount of bytes leaked per test (str -> int)
    totals = {
        'Leak_StillReachable': {'leakedbytes': 0, 'leakedblocks':0},
        'Leak_IndirectlyLost': {'leakedbytes': 0, 'leakedblocks':0}, 
        'Leak_DefinitelyLost': {'leakedbytes': 0, 'leakedblocks':0}, 
        'Leak_PossiblyLost': {'leakedbytes': 0, 'leakedblocks':0}, 
        }

    linecount = 0
    for file in valgrind_files:
        try:
            tree = ET.parse(file)
        except ET.ParseError:
            continue

        root = tree.getroot()
        errors = root.findall("error")
        test = root.findtext("usercomment", default="Test_Unknown test") [len("Test_"):]

        file_totals = {
            'Leak_StillReachable': {'leakedbytes': 0, 'leakedblocks':0},
            'Leak_IndirectlyLost': {'leakedbytes': 0, 'leakedblocks':0}, 
            'Leak_DefinitelyLost': {'leakedbytes': 0, 'leakedblocks':0}, 
            'Leak_PossiblyLost': {'leakedbytes': 0, 'leakedblocks':0}, 
            'total': {'leakedbytes': 0, 'leakedblocks':0}, 
            }

        report = f"<a name=\"{test}\"></a><h3>{test} (<a href=\"#top\">top</a>)</h3>\n"
        # grab the data in the XML
        for error in errors:
            # Only report memleaks
            kind = error.findtext("kind")
            if not kind in ["Leak_StillReachable",  "Leak_IndirectlyLost", "Leak_DefinitelyLost", "Leak_PossiblyLost"]: 
                continue

            file_totals[kind]['leakedbytes'] += int(error.findtext("xwhat/leakedbytes", default="0"))
            file_totals[kind]['leakedblocks'] += int(error.findtext("xwhat/leakedblocks", default="0"))

            if not kind in ["Leak_DefinitelyLost"]: 
                continue

            what = error.findtext("what")
            if not what:
                what = error.findtext("xwhat/text", default="Unknown Error")
            report += f"<h4>{what}</h4>\n"
            for frame in error.findall("stack/frame"):
                dirname = frame.findtext("dir", default="unknown")
                if dirname.startswith("../../"):
                    dirname = dirname[len("../../"):]
                dirname = os.path.normpath(dirname)
                filename = frame.findtext("file", default="unknown")
                line = frame.findtext("line", default="?")
                fn = frame.findtext("fn", default="unknown")
                report += f"  at {dirname}/{filename}:{line} {fn}<br>\n"
            report += "<br>\n"


        for kind in ["Leak_StillReachable",  "Leak_IndirectlyLost", "Leak_DefinitelyLost", "Leak_PossiblyLost"]: 
            file_totals["total"]['leakedbytes'] +=  file_totals[kind]['leakedbytes']
            file_totals["total"]['leakedblocks'] += file_totals[kind]['leakedblocks']

        # Generate the HTML chunck of that test, if needed
        if file_totals["total"]['leakedbytes'] > 0:
            line = {}
            for kind in ["Leak_StillReachable",  "Leak_IndirectlyLost", "Leak_DefinitelyLost", "Leak_PossiblyLost"]:
                line[kind] = f"{file_totals[kind]['leakedbytes']} bytes in {file_totals[kind]['leakedblocks']} blocks"
                if line[kind] == "0 bytes in 0 blocks":
                    line[kind] = '-'
            if line["Leak_DefinitelyLost"] != "-":
                line["Leak_DefinitelyLost"] = f"<a href=\"#{test}\">{line["Leak_DefinitelyLost"]}</a>"
            rows[test] = f"""
                <tr>
                    <td>{test}</td>
                    <td>{line["Leak_DefinitelyLost"]}</td>
                    <td>{line["Leak_IndirectlyLost"]}</td>
                    <td>{line["Leak_PossiblyLost"]}</td>
                    <td>{line["Leak_StillReachable"]}</td>
                </tr>
            """
            if file_totals['Leak_DefinitelyLost']["leakedbytes"] >0:
                reports[test] = report
            leaks_per_test[test] = file_totals["Leak_DefinitelyLost"]['leakedbytes']

    html = f"""
    <html>
    <head>
        <meta charset="utf-8">
        <style>
            table {{
                border-collapse: collapse;
                width: 100%;
                font-family: sans-serif;
            }}
            th, td {{
                border: 1px solid #ccc;
                padding: 8px;
                text-align: left;
            }}
            th {{
                background: #eee;
            }}
            body {{
                font-family: sans-serif;
                margin: 20px;
            }}
        </style>
    </head>
    <body>
        <h1>Valgrind Leak Summary (Aggregated)</h1>

        <h2>Totals</h2>
        <table>
            <tr><th>Category</th><th>Bytes</th><th>Blocks</th></tr>
            <tr><td>Definitely lost</td><td>{totals["Leak_DefinitelyLost"]['leakedbytes']}</td><td>{totals["Leak_DefinitelyLost"]['leakedblocks']}</td></tr>
            <tr><td>Indirectly lost</td><td>{totals["Leak_IndirectlyLost"]['leakedbytes']}</td><td>{totals["Leak_IndirectlyLost"]['leakedblocks']}</td></tr>
            <tr><td>Possibly lost</td><td>{  totals["Leak_PossiblyLost"]['leakedbytes']}</td><td>{  totals["Leak_PossiblyLost"]['leakedblocks']}</td></tr>
            <tr><td>Still reachable</td><td>{totals["Leak_StillReachable"]['leakedbytes']}</td><td>{totals["Leak_StillReachable"]['leakedblocks']}</td></tr>
        </table>

        <h2>Summaries of all tests</h2>
        <table>
    """

    linecount = 0
    for test in sorted(rows, key=lambda t: leaks_per_test[t], reverse=True):
        if linecount == 0:
            html += "<tr><th>Test</th> <th>Definitely lost</th> <th>Indirectly lost</th> <th>Possibly lost</th> <th>Still reachable</th></tr>"
            linecount = 15
        else:
            linecount -= 1
        html += rows[test]

    html += f"""
        </table>
        <h2>Details of each test</h2>
    """
    for test in sorted(reports):
        html += reports[test]

    html += f"""
    </body>
    </html>
    """

    with open(output_html, "w") as f:
        f.write(html)

    print(f"[HTML] Leak report generated: {output_html}")


# -----------------------------
# Main script
# -----------------------------

if __name__ == "__main__":
    valgrind_files = sorted(glob.glob("**/valgrind_test*.xml", recursive=True))

    if not valgrind_files:
        print("No valgrind_*.xml files found in current directory.")
        sys.exit(0)

    print(f"Found {len(valgrind_files)} valgrind XML files.")

    generate_junit(valgrind_files, "valgrind_junit.xml")
    generate_html(valgrind_files, "valgrind_report.html")

    print("Done.")

#!/usr/bin/env python3
"""PLM 用例映射与 gtest XML 后处理（c-native-api-testcase）。"""

import argparse
import json
import os
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MAP_PATH = ROOT / ".plm-case-id-map.json"
DEFAULT_AUTOMATION_TYPE = "session-c"


def load_map():
    if not MAP_PATH.exists():
        return {"nmap": {}, "revmap": {}, "aliases": {}}
    with MAP_PATH.open(encoding="utf-8") as fh:
        data = json.load(fh)
    if "nmap" in data or "revmap" in data:
        return {
            "nmap": data.get("nmap") or {},
            "revmap": data.get("revmap") or {},
            "aliases": data.get("aliases") or {},
        }
    return {"nmap": data, "revmap": {}, "aliases": {}}


def normalize(case_id):
    return str(case_id).strip().upper()


def lookup(case_id):
    data = load_map()
    key = normalize(case_id)
    mapping = {normalize(k): v for k, v in data["nmap"].items()}
    aliases = {normalize(k): v for k, v in data["aliases"].items()}
    target = mapping.get(normalize(aliases.get(key, key)))
    if not target:
        return None
    if isinstance(target, list):
        return target[0] if target else None
    return str(target)


def cmd_list(args):
    data = load_map()
    seen = set()
    entries = []
    for raw_id, targets in data["nmap"].items():
        case_id = normalize(raw_id)
        if case_id in seen:
            continue
        seen.add(case_id)
        target = targets[0] if isinstance(targets, list) and targets else targets
        if not target:
            continue
        target = str(target)
        entries.append(
            {
                "caseId": case_id,
                "automationType": args.automation_type,
                "nodeId": target,
                "sourceFile": "test",
                "description": "",
            }
        )
    entries.sort(key=lambda item: item["caseId"])
    json.dump(entries, sys.stdout, ensure_ascii=False)
    sys.stdout.write("\n")


def cmd_lookup(args):
    target = lookup(args.case_id)
    if not target:
        sys.stderr.write(f"[plm] 未找到用例映射: {args.case_id}\n")
        return 5
    sys.stdout.write(target + "\n")
    return 0


def cmd_junit_post(args):
    source = Path(args.source)
    target = Path(args.target)
    if not source.exists():
        sys.stderr.write(f"[plm] 未找到 gtest 结果: {source}\n")
        return 1
    tree = ET.parse(source)
    root = tree.getroot()
    case_id = normalize(args.case_id)
    count = 0
    for testcase in root.iter("testcase"):
        count += 1
        for existing in list(testcase.findall("properties")):
            for prop in list(existing.findall("property")):
                if prop.get("name") == "case_id":
                    existing.remove(prop)
            if len(existing) == 0:
                testcase.remove(existing)
        properties = testcase.find("properties")
        if properties is None:
            properties = ET.Element("properties")
            testcase.insert(0, properties)
        ET.SubElement(properties, "property", {"name": "case_id", "value": case_id})
    if count == 0:
        sys.stderr.write("[plm] gtest XML 中没有 testcase 节点\n")
        return 1
    target.parent.mkdir(parents=True, exist_ok=True)
    tmp = target.with_suffix(target.suffix + ".tmp")
    tree.write(tmp, encoding="utf-8", xml_declaration=True)
    os.replace(tmp, target)
    sys.stderr.write(f"[plm] 已写出 {count} 个 testcase 到 {target}\n")
    return 0


def main():
    parser = argparse.ArgumentParser()
    sub = parser.add_subparsers(dest="command", required=True)

    p_list = sub.add_parser("list")
    p_list.add_argument(
        "--automation-type",
        default=os.environ.get("PLM_AUTOMATION_TYPE", DEFAULT_AUTOMATION_TYPE),
    )
    p_list.set_defaults(func=cmd_list)

    p_lookup = sub.add_parser("lookup")
    p_lookup.add_argument("case_id")
    p_lookup.set_defaults(func=cmd_lookup)

    p_post = sub.add_parser("junit-post")
    p_post.add_argument("source")
    p_post.add_argument("target")
    p_post.add_argument("case_id")
    p_post.set_defaults(func=cmd_junit_post)

    args = parser.parse_args()
    sys.stdout.reconfigure(encoding="utf-8")
    sys.stderr.reconfigure(encoding="utf-8")
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())

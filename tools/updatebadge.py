#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import re

def readme(fname, new_version):
    badge = re.compile(rb'(\[!\[.*?\]\(https://.*?badge\.(?:svg|png)\?branch=v([^\)]+)\)\])')
    with open(fname, "rb") as f:
        description = f.read()
    re_badge = badge.search(description)
    if re_badge:
        oldbadge = re_badge.group(0)
        newbadge = oldbadge.replace(re_badge.group(2), new_version.encode())
        if oldbadge != newbadge:
            with open(fname, "wb") as w:
                w.write(description.replace(oldbadge, newbadge))

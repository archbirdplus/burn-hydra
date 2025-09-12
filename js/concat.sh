#!/bin/bash

cat header $(find ../trace -name "rank[0-9].json" | sort) footer > rankall.json


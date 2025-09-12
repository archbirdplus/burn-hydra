#!/bin/bash

cat header $(find ../trace -name "rank[0-9].json") footer > rankall.json


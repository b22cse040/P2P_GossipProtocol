#!/bin/bash
truncate -s 0 graph.txt
sleep 0.1            # Small delay to ensure the change is registered
g++ simulation.cpp -o sim3 -pthread
./sim3

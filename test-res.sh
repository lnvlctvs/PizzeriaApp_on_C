#!/bin/bash

gcc pizza.c -o pizza -pthread

./pizza 100 1000

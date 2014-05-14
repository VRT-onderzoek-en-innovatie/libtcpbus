#!/bin/bash

./tcp-bus &
PID=$!

sleep 1;

kill -INT $PID
exit $?

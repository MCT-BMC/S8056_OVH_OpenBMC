#!/bin/bash

check_service=$1
restart_service=$2
default_sleep=$3
retry=$4
mapper_service="/usr/bin/env mapper"
systemctl_service="/usr/bin/env systemctl"

sleep $default_sleep

for i in $(eval echo {1..$retry}); do
    $mapper_service get-service $check_service
    status=$($mapper_service get-service $check_service)
    if [ -n "$status" ]; then
        exit 0
    fi
    sleep 1
done

$systemctl_service restart $restart_service
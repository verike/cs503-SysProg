#!/usr/bin/env bats

# File: student_tests.sh
# 
# Create your unit tests suit in this file

@test "Example: check ls runs without errors" {
    run ./dsh <<EOF                
ls
EOF

    # Assertions
    [ "$status" -eq 0 ]
}


@test "Execute echo command with arguments" {
    run ./dsh <<EOF
echo Hello World
EOF
    [ "$status" -eq 0 ]
    [[ "$output" == *"Hello World"* ]]
}

@test "Invalid command returns error" {
    run ./dsh <<EOF
invalidcmd
EOF
    [ "$status" -ne 127 ]
    [[ "$output" == *"error"* ]]
}

@test "Built-in cd command with path" {
    run ./dsh <<EOF
cd /tmp
pwd
EOF
    [[ "$output" == *"/tmp"* ]]
}

@test "Built-in rc command prints last return code" {
    run ./dsh <<EOF
echo test
rc
EOF
    [[ "$output" == *"0"* ]]
}

@test "Handle spaces in quoted arguments" {
    run ./dsh <<EOF
echo "this is a test"
EOF
    [[ "$output" == *"this is a test"* ]]
}

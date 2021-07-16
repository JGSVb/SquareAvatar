#!/bin/bash

CC=gcc
FLAGS="-g3 -Wall -Wextra -Wpedantic"
GTK=$(pkg-config gtk+-3.0 --cflags --libs)

SOURCES="main.c application.c mainwindow.c editor.c savebutton.c utils.c"
OBJ_DIR=obj
OUTPUT=main

function get_file_output() { echo $OBJ_DIR/"${1%.c}".o; }

function get_file_compile_command() {
	echo "$CC $FLAGS -c $1 -o $(get_file_output "$1") $GTK"
}

function setup() {
	mkdir -p $OBJ_DIR
	rm $OBJ_DIR/*.o
}

function exit_if_fail() {

	eval $1
	
	RET=$?

	if [ $RET != 0 ]
	then
		printf "\033[1mERRO: \033[0;31mO código de saída foi %d, abortando ...\033[0m\n" $RET
		exit $RET
	fi
}

function build() {
	for src in $SOURCES
	do
		exit_if_fail "$(get_file_compile_command "$src")"	
	done
}

setup
build

exit_if_fail "$CC $FLAGS $OBJ_DIR/*.o -o $OUTPUT -lm $GTK"

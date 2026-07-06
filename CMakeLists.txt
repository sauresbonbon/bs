cmake_minimum_required(VERSION 3.18)
project(bs_rein C)

set(CMAKE_C_STANDARD 11)

include_directories(hausuebung/hu1/bshell/src)
include_directories(hausuebung/hu2/memory/src)

add_executable(bs_rein
        hausuebung/hu1/bshell/src/command.c
        hausuebung/hu1/bshell/src/command.h
        hausuebung/hu1/bshell/src/debug.h
        hausuebung/hu1/bshell/src/execute.c
        hausuebung/hu1/bshell/src/execute.h
        hausuebung/hu1/bshell/src/helper.c
        hausuebung/hu1/bshell/src/helper.h
        hausuebung/hu1/bshell/src/list.c
        hausuebung/hu1/bshell/src/list.h
        hausuebung/hu1/bshell/src/readlineparsing.c
        hausuebung/hu1/bshell/src/shell.c
        hausuebung/hu1/bshell/src/shell.h
        hausuebung/hu1/bshell/src/statuslist.c
        hausuebung/hu1/bshell/src/statuslist.h
        hausuebung/hu1/bshell/src/stringbuffer.c
        hausuebung/hu1/bshell/src/stringbuffer.h
        hausuebung/hu1/bshell/src/tokenparser.c
        hausuebung/hu1/bshell/src/tokenparser.h
        hausuebung/hu1/bshell/src/tokenscanner.c
        hausuebung/hu1/bshell/src/types.h
        hausuebung/hu2/memory/src/main.c
        hausuebung/hu2/memory/src/output_utility.c
        hausuebung/hu2/memory/src/output_utility.h
        hausuebung/hu2/memory/src/vmm.c
        hausuebung/hu2/memory/src/vmm.h)

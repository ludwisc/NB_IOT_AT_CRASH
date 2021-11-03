#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf-coap-client)

# NORDIC SDK APP START
target_sources(app PRIVATE 
src/main.c
src/lte_communication.c
src/fsm.c
src/radar_communication.c
src/measure.c
)

# NORDIC SDK APP END

target_include_directories(app PRIVATE inc)

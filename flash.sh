#!/bin/bash

echo Attempting to flash image

JLinkExe -device stm32f103c8 -speed 1000 -if swd -autoconnect 1 -CommanderScript flash.jlink

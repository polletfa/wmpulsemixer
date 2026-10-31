#!/usr/bin/env bash
docker run --volume .:/pkg -ti archlinux:makepkg bash

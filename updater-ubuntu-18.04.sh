#!/bin/bash

echo "Citra Installer & Updater for Ubuntu 18.04"
echo "Checking Ubuntu version..."
exitversion () {
    echo "Ubuntu version is not 18.04"
    exit 1
}
cat /etc/lsb-release | grep "DISTRIB_RELEASE=18.04">/dev/null || exitversion
echo "Checking Ubuntu version... 18.04"
repo="$1"
install=false
if [ -z "$repo" ]; then
    echo "Directory argument is empty"
    repo="$HOME/citra"
    echo "Checking for $repo..."
    if [ -d "$repo" ]; then
        echo "Checking for $repo... Found"
        echo "Repository directory set to $repo"
    else
        echo "Checking for $repo... Not found"
        install=true
    fi
else
    echo "Checking for $repo..."
    if [ -d "$repo" ]; then
        echo "Checking for $repo... Found"
        echo "Repository directory set to $repo"
    else
        echo "Checking for $repo... Not found"
        install=true
    fi
fi
if $install; then
    echo "Cloning to $repo"
    git clone --recursive https://github.com/citra-valentin/citra $repo
    echo "Entering $repo"
    cd "$repo"
    echo "Adding upstream remote"
    git remote add upstream https://github.com/citra-valentin/citra
    echo "Creating build directory"
    mkdir build
    echo "Entering build directory"
    cd build
else
    echo "Entering $repo"
    cd "$repo"
    echo "Running git checkout master"
    git checkout master
    echo "Running git reset --hard HEAD"
    git reset --hard HEAD
    echo "Running git pull"
    git pull
    if [ ! -d "build" ]; then
        echo "Build directory not found, creating it"
        mkdir build
    fi
    echo "Entering build directory"
    cd build
fi
echo "Running cmake"
cmake .. -DCMAKE_BUILD_TYPE=Release -DENABLE_DISCORD_RPC=ON
echo "Compiling"
make -j8
RESULT=$?
if [ $RESULT -eq 0 ]; then
    echo "Build succeeded"
    echo "Opening Citra"
    ./bin/citra
else
    echo "Build failed"
fi

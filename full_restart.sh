#!/usr/bin/env bash
docker-compose stop
sudo rm -rf ./experience/*
docker-compose up -d --force-recreate
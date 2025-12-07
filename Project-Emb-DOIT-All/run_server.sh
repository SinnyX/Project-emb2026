#!/bin/bash

# Script to run FastAPI server

echo "Installing dependencies..."
pip install -r requirements.txt

echo "Starting FastAPI server..."
python fastapi_server.py


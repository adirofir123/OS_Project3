# 📐 OS Project 3 — Convex Hull Server with Reactor/Proactor

A multi-threaded geometric graph server in C that computes convex hulls, supports multi-client input, and is built with both Reactor and Proactor patterns.

## 🧠 Overview

This project implements a **dynamic server** that:
- Accepts point sets from clients
- Computes the **convex hull** using Graham's scan (Monotone Chain)
- Calculates the area
- Synchronizes access across multiple clients and threads

## 🧱 Project Stages

### Stage 1: Convex Hull
- Reads points from standard input
- Computes convex hull and prints area

### Stage 2: Profiling
- Benchmarks `deque` vs `list` performance for CH

### Stage 3: Interactive Input
- Commands:
  - `Newgraph n` → reads `n` new points
  - `Newpoint x,y` → adds point
  - `Removepoint x,y` → removes point
  - `CH` → computes convex hull

### Stage 4: Multi-User Graph Sharing
- Clients interact with a shared graph over TCP
- Each client can modify or compute CH

### Stage 5: Reactor Pattern
- Custom-built `reactor` using `select()`
- Manages FDs and functions via:
  ```c
  addFdToReactor(), removeFdFromReactor(), startReactor()

"""Turns the experiment CSV files into PNG charts for the report.

Usage (after running the experiments program):
    python experiments/plot_results.py            # reads experiments/results/*.csv
    python experiments/plot_results.py --in DIR   # another results folder

Needs matplotlib:  pip install matplotlib
Charts are written to <results>/plots/. Edit freely: this is a starting point.
"""
import argparse
import csv
import math
import os
from collections import defaultdict

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402

# Fixed colour order (colour-blind checked), always assigned in this order.
SERIES = ["#2a78d6", "#eb6834", "#1baf7a", "#eda100", "#e87ba4", "#008300", "#6250d6", "#e34948"]
INK, INK_SOFT, GRID = "#0b0b0b", "#52514e", "#e4e3df"


def style(ax, title, xlabel, ylabel):
    ax.set_title(title, loc="left", fontsize=12, color=INK, pad=10)
    ax.set_xlabel(xlabel, color=INK_SOFT)
    ax.set_ylabel(ylabel, color=INK_SOFT)
    ax.grid(True, color=GRID, linewidth=0.8)
    ax.set_axisbelow(True)
    for side in ("top", "right"):
        ax.spines[side].set_visible(False)
    for side in ("left", "bottom"):
        ax.spines[side].set_color(GRID)
    ax.tick_params(colors=INK_SOFT)


def read(path):
    with open(path, newline="") as f:
        return list(csv.DictReader(f))


def save(fig, out, name):
    fig.tight_layout()
    path = os.path.join(out, name)
    fig.savefig(path, dpi=160)
    plt.close(fig)
    print("wrote", path)


def plot_sorting(rows, out):
    fig, ax = plt.subplots(figsize=(7.5, 4.6))
    algos = ["bubble", "selection", "insertion", "merge", "quick", "std::sort"]
    for i, algo in enumerate(algos):
        pts = sorted((int(r["n"]), int(r["comparisons"])) for r in rows if r["algorithm"] == algo and r["input"] == "random")
        if not pts:
            continue
        xs, ys = zip(*pts)
        label = algo + (" (hidden under selection: same count)" if algo == "bubble" else "")
        ax.plot(xs, ys, marker="o", markersize=5, linewidth=2, color=SERIES[i], label=label)
    ax.set_xscale("log")
    ax.set_yscale("log")
    style(ax, "E1  Comparisons on random input (log-log)", "n (items)", "comparisons")
    ax.legend(frameon=False, fontsize=9, loc="upper left")
    save(fig, out, "e1_sorting_comparisons.png")


def plot_pathfinding(rows, out):
    groups = defaultdict(lambda: [0, 0, 0])
    for r in rows:
        g = groups[r["obstacles"]]
        g[0] += int(r["bfs_expanded"])
        g[1] += int(r["astar_expanded"])
        g[2] += 1
    labels = sorted(groups, key=int)
    bfs = [groups[k][0] / groups[k][2] for k in labels]
    astar = [groups[k][1] / groups[k][2] for k in labels]
    fig, ax = plt.subplots(figsize=(6.5, 4.2))
    width = 0.36
    xs = range(len(labels))
    b1 = ax.bar([x - width / 2 - 0.01 for x in xs], bfs, width, color=SERIES[0], label="BFS")
    b2 = ax.bar([x + width / 2 + 0.01 for x in xs], astar, width, color=SERIES[1], label="A*")
    for bars in (b1, b2):
        for bar in bars:
            ax.annotate(f"{bar.get_height():.0f}", (bar.get_x() + bar.get_width() / 2, bar.get_height()),
                        xytext=(0, 3), textcoords="offset points", ha="center", fontsize=9, color=INK)
    ax.set_xticks(list(xs))
    ax.set_xticklabels([f"{k} obstacles" for k in labels])
    style(ax, "E3  Cells expanded per path (1000 random pairs)", "", "cells expanded (mean)")
    ax.legend(frameon=False)
    save(fig, out, "e3_bfs_vs_astar.png")


def plot_routing(rows, out):
    by_k = defaultdict(list)
    for r in rows:
        by_k[int(r["k"])].append(r)
    ks = sorted(by_k)
    mean = lambda k, col: sum(float(r[col]) for r in by_k[k]) / len(by_k[k])  # noqa: E731
    fig, ax = plt.subplots(figsize=(7, 4.4))
    for i, (col, label) in enumerate([("as_given", "as given"), ("greedy", "greedy NN"), ("backtracking", "backtracking (exact)")]):
        ys = [mean(k, col) for k in ks]
        ax.plot(ks, ys, marker="o", markersize=5, linewidth=2, color=SERIES[i], label=label)
        ax.annotate(label, (ks[-1], ys[-1]), xytext=(6, 0), textcoords="offset points", va="center", fontsize=9, color=INK_SOFT)
    style(ax, "E4  Route length vs shelves per trip", "k (shelves)", "moves (mean of 100 trips)")
    ax.legend(frameon=False, fontsize=9)
    save(fig, out, "e4_route_length.png")

    fig, ax = plt.subplots(figsize=(7, 4.4))
    pruned = [mean(k, "nodes_pruned") for k in ks]
    ax.plot(ks, pruned, marker="o", markersize=5, linewidth=2, color=SERIES[0], label="with pruning")
    full_ks = [k for k in ks if any(int(r["nodes_full"]) >= 0 for r in by_k[k])]
    full = []
    for k in full_ks:
        vals = [int(r["nodes_full"]) for r in by_k[k] if int(r["nodes_full"]) >= 0]
        full.append(sum(vals) / len(vals))
    ax.plot(full_ks, full, marker="o", markersize=5, linewidth=2, color=SERIES[1], label="no pruning")
    ax.plot(ks, [math.factorial(k) * 2 ** k for k in ks], linewidth=1.5, linestyle="--", color=INK_SOFT, label="k! x 2^k (leaves of the full tree)")
    ax.set_yscale("log")
    style(ax, "E4  Backtracking search-tree nodes (log scale)", "k (shelves)", "nodes visited (mean)")
    ax.legend(frameon=False, fontsize=9)
    save(fig, out, "e4_search_nodes.png")


def plot_traffic(rows, out):
    by_p = defaultdict(list)
    for r in rows:
        by_p[int(r["pickers"])].append(float(r["makespan"]))
    ps = sorted(by_p)
    makespan = [sum(by_p[p]) / len(by_p[p]) for p in ps]
    fig, ax = plt.subplots(figsize=(7, 4.4))
    ax.plot(ps, [makespan[0] / m for m in makespan], marker="o", markersize=5, linewidth=2, color=SERIES[0], label="measured speed-up")
    ax.plot(ps, ps, linewidth=1.5, linestyle="--", color=INK_SOFT, label="ideal (no congestion)")
    style(ax, "E5  Speed-up vs number of picking robots (80 orders)", "picking robots", "speed-up vs 1 robot")
    ax.legend(frameon=False, fontsize=9)
    save(fig, out, "e5_speedup.png")


def plot_policies(rows, out):
    groups = defaultdict(list)
    for r in rows:
        groups[(r["routing"], r["assignment"])].append(float(r["moves"]))
    keys = list(dict.fromkeys((r["routing"], r["assignment"]) for r in rows))
    vals = [sum(groups[k]) / len(groups[k]) for k in keys]
    fig, ax = plt.subplots(figsize=(8, 4.4))
    colors = [SERIES[0] if a.startswith("First") else SERIES[1] for _, a in keys]
    bars = ax.bar(range(len(keys)), vals, 0.7, color=colors)
    for bar in bars:
        ax.annotate(f"{bar.get_height():.0f}", (bar.get_x() + bar.get_width() / 2, bar.get_height()), xytext=(0, 3),
                    textcoords="offset points", ha="center", fontsize=9, color=INK)
    ax.set_xticks(range(len(keys)))
    ax.set_xticklabels([f"{r}\n{a}" for r, a in keys], fontsize=8)
    style(ax, "E6  Distance driven for 80 orders (mean of 10 seeds)", "routing / robot choice", "cells driven")
    from matplotlib.patches import Patch
    ax.legend(handles=[Patch(color=SERIES[0], label="first available robot"), Patch(color=SERIES[1], label="nearest robot")],
              frameon=False, fontsize=9)
    save(fig, out, "e6_policies.png")


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    parser = argparse.ArgumentParser()
    parser.add_argument("--in", dest="folder", default=os.path.join(here, "results"))
    args = parser.parse_args()
    out = os.path.join(args.folder, "plots")
    os.makedirs(out, exist_ok=True)
    jobs = [("e1_sorting.csv", plot_sorting), ("e3_pathfinding.csv", plot_pathfinding), ("e4_routing.csv", plot_routing),
            ("e5_traffic.csv", plot_traffic), ("e6_policies.csv", plot_policies)]
    for name, fn in jobs:
        path = os.path.join(args.folder, name)
        if os.path.exists(path):
            fn(read(path), out)
        else:
            print("skipped", name, "(run the experiments first)")


if __name__ == "__main__":
    main()

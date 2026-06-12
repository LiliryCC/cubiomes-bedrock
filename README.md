<img width="4000" height="2250" alt="3d61b2c1e7f9849acbe3ac7ccb1ae53d" src="https://github.com/user-attachments/assets/09fe98cd-de63-40f2-a89b-0b6a6780dca4" />

-------

This is an application fork of [this epic bedrock C Library](https://github.com/FragrantResult186/cubiomes-bedrock) for calculating Stronghold Village Rates, which helps make optimal routing decisions in Bedrock speedrunning. The data is based on `MC_1_21`, but I assume it's structurally applicable to every version later than `MC_1_14`.

The original core algorithms are ported from the incredible [Minecraft biome generation C Library](https://github.com/Cubitect/cubiomes) by [Cubitect](https://github.com/Cubitect).

Massive thanks to the original authors!

## What is this

In Bedrock RSG speedrunning, gambling for a Stronghold Village is widely considered the fastest strategy. However, the chance of hitting the jackpot is relatively low. The true generation rate of a Stronghold Village in a specific grid is defined as:

$$P = \frac{N_{\text{Village with Stronghold}}}{N_{\text{Village}}}$$

We already know this rate is strongly correlated with the coordinates (x, z). Yet for modern versions, there has been a lack of hard data to prove whether the probability in a specific grid is actually higher or lower since the spawnpoint algorithms have changed (in 1.18 I assume). That's why I built this to help.

The code will scan the coordinates in precise grids (32x32 blocks, or 2x2 chunks by default) and simulate massive amounts of PRNG cycles. To ensure statistical confidence without wasting CPU power, it utilizes an **Adaptive Truncated Sampling Logic**:

* **The Ceiling:** It blasts through up to 2,000,000 seeds per grid to guarantee accurate data even in barren areas.
* **Early Truncation:** The moment the engine finds exactly **300 valid villages** in a single grid, it immediately stops searching and calculates the rate. This ensures that high-density areas are processed instantly while maintaining an equal statistical weight across the entire map.
* **The "Village Dead Zone" Interceptor:** Bedrock Edition calculates village generation within a 34x34 chunk grid (544x544 blocks). However, to prevent structure overlap and heavy AI pathfinding lag (I assume), the developers hardcoded a physical buffer: the game restricts the placement by using `nextInt(34 - 8)`. This means in *every* 544-block cycle across the entire infinite world, **the last 8 chunks (128 blocks) are physically forbidden from generating a village.** (For example, the coordinate intervals `[-128, 0]` and `[416, 544]`). It acts like an invisible net cutting the world into isolated islands. Therefore, instead of wasting millions of PRNG cycles in these desolate grids, I implemented an $O(1)$ mathematical interceptor. If a scanning bounding box falls entirely within these 128-block strips, the radar instantly marks it as `0.0%` (represented as an `X` in the terminal) and moves on. This simple pruning logic dramatically boosts the global scanning speed!

...and of course, you can modify these parameters if you wish.

## How to Use

1. Build the project using CMake.
2. Run the executable compiled from `1_heatmap.c`.
3. You can also modify the following parameters in the code to explore different regions and precision levels:
   * **Bounding Box:** `start_x`, `end_x`, `start_z`, `end_z`
   * **Resolution:** `step_x`, `step_z` (Default is 32 blocks, optimized for Bedrock's chunk math)
   * **Confidence Levels:** `TARGET_SAMPLES` and `max_seeds`
4. Alternatively, if you don't want to run the code yourself, you can simply takeaway that provided heatmap and use it. Hope it helps!

Also, if you are only curious about the rate of a specific grid (instead of scanning over a large area), you could try changing the parameters and running the `1_shv_rate_for_specific_square.c` file.

## What You Will Get

A `.csv` file containing the probability of a village being a Stronghold Village within your specified grid. You can import this data into Excel and apply conditional formatting to generate a highly readable heatmap plot like this:

<img width="1755" height="1241" alt="heatmap" src="https://github.com/user-attachments/assets/30458c20-4653-4942-9945-024b99e2b1d8" />

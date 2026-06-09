This is a application fork of [This epic bedrock C Libriary](https://github.com/FragrantResult186/cubiomes-bedrock) for searching Stronghold Village Rates, which helps making desicion in bedrock speedrunning. The data based on `MC_1_21`, but I assume its suitable for every verison later than 1.14

The original core algorithms are ported from the incredible [Minecraft biome generation C Libriry](https://github.com/Cubitect/cubiomes) by [Cubitect](https://github.com/Cubitect).

Massive thanks to the original authors!

# How to Use

1. Build the project using CMake.
2. Run the executable compiled from `1_heatmap.c`.
3. You can also modify the following parameters to explore different regions and precision levels:
   * **Bounding Box:** `start_x`, `end_x`, `start_z`, `end_z`
   * **Resolution:** `step_x`, `step_z` (Default is 32 blocks, optimized for Bedrock's chunk math)
   * **Confidence Levels:** `TARGET_SAMPLES` and `max_seeds`
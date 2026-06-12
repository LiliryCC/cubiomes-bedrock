#include "generator.h"
#include "finders.h"
#include <stdio.h>
#include <string.h>
#include <omp.h>
#include <stdlib.h> 
#include <windows.h> 

// ==========================================
// Dead Zone of Villiage Generation (every 544 blocks zone, from 416 to 543)
// 544 的周期性死区，导致在某些特定范围内完全没有村庄生成
// ==========================================
int is_box_dead(int min_val, int max_val) {
    int actual_max = max_val - 1; 

    int offset_min = min_val % 544;
    if (offset_min < 0) offset_min += 544;
    
    int offset_max = actual_max % 544;
    if (offset_max < 0) offset_max += 544;

    int period_min = (min_val < 0) ? (min_val - 543) / 544 : min_val / 544;
    int period_max = (actual_max < 0) ? (actual_max - 543) / 544 : actual_max / 544;

    if (period_min == period_max && offset_min >= 416 && offset_max >= 416) {
        return 1; 
    }
    return 0; 
}

int main() {
    SetConsoleOutputCP(CP_UTF8);

    Generator g;
    setupGenerator(&g, MC_1_21, 0);

    uint64_t max_seeds = 2000000; 
    const int TARGET_SAMPLES = 300; 

    // ==========================================
    // Setup of XZ grid for heatmap
    // The step sizes should be chosen to align with the village generation patterns to ensure we capture the dead zones effectively.
    // XZ 轴网格配置
    // 数值最好设置成32的整数倍，以保证与步长对齐，避免边界问题
    // ==========================================

    // X
    int start_x = -1024;
    int end_x = 1024;
    int step_x = 32;
    
    // Z
    int start_z = -1024;
    int end_z = 1024;
    int step_z = 32;

    // Calculate grid sizes based on the defined ranges and steps
    // 根据定义的范围和步长计算网格大小
    int grid_size_x = (end_x - start_x) / step_x; 
    int grid_size_z = (end_z - start_z) / step_z; 
    double heatmap[grid_size_z][grid_size_x];

    printf("========================================================\n");
    printf("  Ultimate FSG Radar: Rectangular Heatmap Engine        \n");
    printf("========================================================\n");
    printf("-> X: [%d, %d] | Steps: %d | Columns: %d\n", start_x, end_x, step_x, grid_size_x);
    printf("-> Z: [%d, %d] | Steps: %d | Rows: %d\n", start_z, end_z, step_z, grid_size_z);
    printf("-> Total Matrix Size: %d Detection Units\n", grid_size_x * grid_size_z);
    printf("========================================================\n");

    double global_start_time = omp_get_wtime();

    // 外层循环：遍历 Z 轴网格 (行)
    for (int zi = 0; zi < grid_size_z; zi++) {
        int min_block_z = start_z + zi * step_z;
        int max_block_z = min_block_z + step_z;

        // 内层循环：遍历 X 轴网格 (列)
        for (int xi = 0; xi < grid_size_x; xi++) {
            int min_block_x = start_x + xi * step_x;
            int max_block_x = min_block_x + step_x;

            // 调用Dead Zone检测函数，快速跳过完全没有村庄生成的区域
            if (is_box_dead(min_block_x, max_block_x) || is_box_dead(min_block_z, max_block_z)) {
                heatmap[zi][xi] = 0.0; 
                printf("X"); 
                fflush(stdout);
                continue; 
            }

            int hit_count = 0;      
            int total_villages = 0; 
            int stop_searching = 0;

            int min_rx = (min_block_x / 544) - 1;
            int max_rx = (max_block_x / 544) + 1;
            int min_rz = (min_block_z / 544) - 1;
            int max_rz = (max_block_z / 544) + 1;

            #pragma omp parallel for schedule(dynamic, 100)
            for (uint64_t seed = 1; seed <= max_seeds; seed++) {
                if (stop_searching) continue;

                Generator local_g = g; 
                applySeed(&local_g, DIM_OVERWORLD, seed);
                
                int local_villages = 0;
                int has_village_in_box = 0; 
                
                for (int rx = min_rx; rx <= max_rx; rx++) {
                    for (int rz = min_rz; rz <= max_rz; rz++) {
                        Pos v_pos;
                        if (getStructurePos(Village, local_g.mc, seed, rx, rz, &v_pos)) {
                            if (v_pos.x >= min_block_x && v_pos.x <= max_block_x &&
                                v_pos.z >= min_block_z && v_pos.z <= max_block_z) {
                                if (isViableStructurePos(Village, &local_g, v_pos.x, v_pos.z, 0)) {
                                    local_villages++;
                                    has_village_in_box = 1; 
                                }
                            }
                        }
                    }
                }

                if (local_villages > 0 && !stop_searching) {
                    int current_total;
                    #pragma omp atomic capture
                    {
                        total_villages += local_villages;
                        current_total = total_villages;
                    }
                    if (current_total >= TARGET_SAMPLES) {
                        stop_searching = 1; 
                    }
                }

                if (has_village_in_box) {
                    StrongholdIter sh;
                    memset(&sh, 0, sizeof(sh)); 
                    for (int i = 0; i < 3; i++) {
                        nextVillageStronghold(&sh, &local_g);
                        int v_x = sh.pos.x + 4;
                        int v_z = sh.pos.z + 4;
                        if (v_x >= min_block_x && v_x <= max_block_x &&
                            v_z >= min_block_z && v_z <= max_block_z) {
                            if (!stop_searching) {
                                #pragma omp atomic
                                hit_count++;
                            }
                        }
                    }
                }
            }

            double rate = 0.0;
            if (total_villages > 0) {
                rate = ((double)hit_count / total_villages) * 100.0;
            }
            heatmap[zi][xi] = rate;

            // 标记村庄频率 Notation for Village Frequency
            if (stop_searching) {
                printf("5"); 
            } else {
                if (total_villages > 150) printf("4");
                else if (total_villages >= 51 && total_villages <= 150) printf("3");
                else if (total_villages >= 11 && total_villages <= 50) printf("2");
                else if (total_villages >= 11 && total_villages <= 50) printf("2");
                else if (total_villages >= 1 && total_villages <= 10) printf("1");
                else printf("0"); 
            }
            fflush(stdout); 
        }
        printf(" (Z=[%d,%d] done)\n", min_block_z, max_block_z); 
    }

    double global_time_spent = omp_get_wtime() - global_start_time;

    // ==========================================
    // 数据导出为 CSV 文件
    // Output the heatmap data to a CSV file
    // ==========================================
    FILE *csv_file = fopen("heatmap.csv", "w");
    if (csv_file == NULL) {
        printf("\n[错误] 无法创建 CSV 文件！\n");
        return 1;
    }

    fprintf(csv_file, "\xEF\xBB\xBF");

    // 写入 CSV 表头 (利用 grid_size_x)
    fprintf(csv_file, "Z \\ X");
    for (int xi = 0; xi < grid_size_x; xi++) {
        fprintf(csv_file, ",\"[ %d, %d ]\"", start_x + xi * step_x, start_x + (xi + 1) * step_x);
    }
    fprintf(csv_file, "\n");

    // 写入 CSV 矩阵数据 (外层 grid_size_z，内层 grid_size_x)
    for (int zi = 0; zi < grid_size_z; zi++) {
        fprintf(csv_file, "\"[ %d, %d ]\"", start_z + zi * step_z, start_z + (zi + 1) * step_z);
        
        for (int xi = 0; xi < grid_size_x; xi++) {
            double rate = heatmap[zi][xi];
            fprintf(csv_file, ",%.1f", rate);
        }
        fprintf(csv_file, "\n");
    }
    fclose(csv_file);

    printf("\n============= 最终报告 =============\n");
    printf("Time Cost: %.2f s\n", global_time_spent);
    printf("-> %dx%d CSV 数据已导出！\n", grid_size_x, grid_size_z);
    printf("-> 请前往当前项目文件夹下的 [ heatmap.csv ] 文件查看\n");
    printf("=========================================================\n");

    return 0;
}
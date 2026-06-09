#include "generator.h"
#include "finders.h"
#include <stdio.h>
#include <string.h>
#include <omp.h>

// ==========================================
// 村庄死区拦截器 (Dead Zone Interceptor for Villages)
// ==========================================
int is_dead_zone(int min_p, int max_p) {
    int m_min = (min_p % 544 + 544) % 544;
    
    // 2. 如果起点根本不在死区 (416~543)，直接出局
    if (m_min < 416) return 0;
    
    // 3. 计算从起点到本周期死区尽头 (543) 还剩多少格
    int max_allowed_dist = 543 - m_min;
    
    // 4. 边界特例容错：
    // 如果你传入的 max_p 恰好是 544 的倍数 (下一个周期的第0格)，
    // 我们包容这种“以 544 为边界”的写法，允许距离上限 +1
    if (max_p > min_p && max_p % 544 == 0) {
        max_allowed_dist += 1;
    }
    
    // 5. 如果目标框的总宽度，没有超过允许的死区剩余距离，说明它被完全吞没！
    if (max_p - min_p <= max_allowed_dist) {
        return 1;
    }
    
    return 0;
}
int main() {
    Generator g;
    setupGenerator(&g, MC_1_21, 0);

    uint64_t start_seed = 1;
    uint64_t end_seed = 500000; 
    
    int hit_count = 0;      
    int total_villages = 0; 

    // 你要狙击的目标框坐标
    int min_block_x = -100;
    int max_block_x = 0;
    int min_block_z = -100;
    int max_block_z = 0;

    printf("Calculating TRUE Stronghold Village probability...\n");
    printf("Target Box: X[%d to %d], Z[%d to %d]\n\n", min_block_x, max_block_x, min_block_z, max_block_z);

    // ==========================================
    // 死区判定：只要 X 轴或 Z 轴完全掉入死区，整个框都无法刷出村庄
    // ==========================================
    if (is_dead_zone(min_block_x, max_block_x) || is_dead_zone(min_block_z, max_block_z)) {
        printf("[INTERCEPTED] The target box is entirely within the 128-block Village Dead Zone!\n");
        printf("-> Pruned %llu PRNG cycles instantly.\n", end_seed - start_seed + 1);
        printf("============= EXPEDITION COMPLETED =============\n");
        printf("-> Stronghold Village Rate: 0.0000%%\n");
        printf("------------------------------------------------\n");
        printf("-> Total Execution Time: 0.000 seconds\n");
        return 0; // 直接结束程序，保全算力！
    }

    printf("Scanning seeds %llu to %llu...\n\n", start_seed, end_seed);

    // 开始记录绝对时间
    double start_time = omp_get_wtime();

    #pragma omp parallel for
    for (uint64_t seed = start_seed; seed <= end_seed; seed++) {
        Generator local_g = g; 
        applySeed(&local_g, DIM_OVERWORLD, seed);
        
        int local_villages = 0;
        int has_village_in_box = 0; 
        
        // 第一道大门：检查目标框内是否有村庄
        for (int rx = 0; rx <= 2; rx++) {
            for (int rz = 0; rz <= 2; rz++) {
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

        if (local_villages > 0) {
            #pragma omp atomic
            total_villages += local_villages;
        }

        // 第二道门槛：只有目标框内确实有村庄的种子，才去生成要塞
        if (has_village_in_box) {
            StrongholdIter sh;
            memset(&sh, 0, sizeof(sh)); 

            for (int i = 0; i < 3; i++) {
                nextVillageStronghold(&sh, &local_g);
                
                int v_x = sh.pos.x + 4;
                int v_z = sh.pos.z + 4;
                
                if (v_x >= min_block_x && v_x <= max_block_x &&
                    v_z >= min_block_z && v_z <= max_block_z) {
                    
                    #pragma omp atomic
                    hit_count++;
                    
                    #pragma omp critical
                    {
                        printf("[HIT] Seed: %llu | Village(X:%d, Z:%d) -> Stronghold(X:%d, Z:%d)\n", 
                               seed, v_x, v_z, sh.pos.x, sh.pos.z);
                    }
                }
            }
        }
    }

    // 结束记录绝对时间
    double end_time = omp_get_wtime();
    double time_spent = end_time - start_time;

    printf("\n============= EXPEDITION COMPLETED =============\n");
    printf("Target Box: X[%d to %d], Z[%d to %d]\n", min_block_x, max_block_x, min_block_z, max_block_z);
    printf("-> Total Validated Villages: %d\n", total_villages);
    printf("-> Stronghold Villages: %d\n", hit_count);
    
    if (total_villages > 0) {
        double rate = ((double)hit_count / total_villages) * 100.0;
        printf("-> Stronghold Village Rate (Stronghold / Village): %.4f%%\n", rate);
    } else {
        printf("-> Stronghold Village Rate: 0.0000%% (No valid villages found)\n");
    }
    
    // 打印最终耗时和每秒吞吐量
    printf("------------------------------------------------\n");
    printf("-> Total Execution Time: %.3f seconds\n", time_spent);
    printf("-> Seeds Processed Per Second: %.0f seeds/sec\n", (end_seed - start_seed + 1) / time_spent);

    return 0;
}
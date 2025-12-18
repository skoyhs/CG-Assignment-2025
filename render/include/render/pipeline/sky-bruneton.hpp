#pragma once

/*
// Layer 1: Initial Computations
T = transmittance_cs()
deltaE = irradiance_1_cs(T)
deltaSR, deltaSM = inscatter_1_cs(T)

// Layer 2: First Accumulation (Initial Copy)
E = copy_irradiance_cs(deltaE, E)
S = copy_inscatter_1_cs(deltaSR, deltaSM)

// Layer 3: Order 2 Computations
deltaJ = inscatter_s_cs(T, deltaE, deltaSR, deltaSM)
deltaE = irradiance_n_cs(deltaSR, deltaSM)
deltaS = inscatter_n_cs(T, deltaJ)

// Layer 4: Order 2 Accumulation (Pure Addition)
E += copy_irradiance_cs(deltaE, E)
S += copy_inscatter_n_cs(S, deltaS)

// Layer 5: Order 3 Computations (Repeat for next scattering order)
S = copy_inscatter_1_cs(deltaS, deltaS)
deltaJ = inscatter_s_cs(T, E, deltaS, deltaS)
deltaE = irradiance_n_cs(deltaS, deltaS)
deltaS = inscatter_n_cs(T, deltaJ)

// Layer 6: Order 3 Accumulation & Save (Pure Addition)
E += copy_irradiance_cs(deltaE, E)
S += copy_inscatter_n_cs(S, deltaS)

save_textures(T, E, S)  // Cache final textures to disk
*/

namespace render::pipeline
{}
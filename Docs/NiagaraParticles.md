# Niagara Particle System
The motion of stars and gas, collectively called "luminous matter", shall be simulated using a particle system. In the Unreal engine the new _Niagara_ particle plugin will be used for this purpose. _Niagara_ replaces the older _Cascade_ system and provides a lot more flexibility using scriptable modules, which will be explored in the process.

## Representing stars with particles
Each particle in the simulation will follow an orbit as described in the paper by Wielen ([Wielen 1974, Density-Wave Theory of the Spiral Structure of Galaxies]()). The main orbital parameters of a stellar orbit are
* Mean distance from galactic center R
* Size of the epicycle r
* 

The mean radius R can be randomly selected. The 

- Calculating stellar orbit
- Kinematic vs. dynamic simulation
## Luminosity
- Naive approach: Each particle is a star
- Efficient approach: Particles with equal luminosity
## Gas clouds and interstellar absorption
## Randomized sampling and floating point precision
- Sampling arbitrary probability distributions
- Converting user-defined probability curves into CDF and quantile functions
- Configuring Niagara systems using curves, textures, parameter collections
## Niagara modules and functions
- Pros/Cons of assets vs. Scratchpad

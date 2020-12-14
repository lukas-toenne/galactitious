# Galaxy Simulator

Simulate galaxies using Niagara particles
- Deep dive into spiral structure mechanisms and stellar orbits
- Learn about the Niagara particle system and how to integrate it in Unreal
- Explore astronomical image rendering

## Galaxies and Theory of Spiral Structure

### Winding problem
### Simple models and resulting stellar orbits
### Stellar life cycles
### Dynamic simulation and Fast Multipole method

## Niagara Particle System

### Representing stars with particles
- Calculating stellar orbit
- Kinematic vs. dynamic simulation
### Luminosity
- Naive approach: Each particle is a star
- Efficient approach: Particles with equal luminosity
### Gas clouds and interstellar absorption
### Randomized sampling and floating point precision
- Sampling arbitrary probability distributions
- Converting user-defined probability curves into CDF and quantile functions
- Configuring Niagara systems using curves, textures, parameter collections
### Niagara modules and functions
- Pros/Cons of assets vs. Scratchpad

## Rendering Stars
- Examples of star and galaxy images
- Brightness and luminosity
- Optical systems and resolution, Airy disk
- Human eyes vs. cameras
- Particle render size from star count

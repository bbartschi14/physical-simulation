![Scene](https://github.com/bbartschi14/physical-simulation/blob/main/in%20action.JPG)
# physical-simulation

An OpenGL scene for physical simulation, including 
- Circular motion (stable trapezoidal integration vs. unstable forward euler)
- Structural spring pendulum
- Cloth (with structural, shear, and flex springs)

Created for MIT course 6.837.

## Running the program

`cloth_sim.exe` can be run in the terminal with two command line parameters that define the integrator (e, t, or r) type and step size. For example, to run with the RK4 integrator and a 0.005 second step size, execute `./cloth_sim.exe r 0.005`. Make sure to have the `assets` folder in the same directory.

## Features

The cloth simulation comes with a number of interactive UI buttons, such as
- Change gravitational force
- Toggle wind and adjust wind strength
- Unpin the cloth from the frame
- Toggle diffuse and normal texture maps
- Visualize normals as colors



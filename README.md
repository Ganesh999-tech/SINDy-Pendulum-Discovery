# SINDy-Pendulum-Discovery
SINDy Pendulum Discovery: Data-Driven Physics Validation

This project demonstrates the discovery of nonlinear pendulum dynamics using Sparse Identification of Nonlinear Dynamics (SINDy). By processing raw gyroscope data from a smartphone (Pixel 10) through MATLAB, the model extracts physical coefficients for gravity, linear friction, and quadratic air drag.
🚀 Overview

The goal of this project is to bridge the gap between raw sensor measurements and textbook physics. Using a simple pendulum setup, we capture angular velocity (ω) via the Phyphox app and use the SINDy algorithm to "discover" the underlying differential equation:
ω˙=Ξ1​+Ξ2​ω+Ξ3​sin(θ)+Ξ4​∣ω∣ω
🛠 Features

    Data Acquisition: Utilizes IMU data (Gyroscope) from smartphone sensors.

    Signal Processing: Implements zero-offset calibration, drift-compensated integration (cumtrapz + detrend), and numerical differentiation.

    Sparse Regression: Uses a least-squares solver with a sparsity threshold to identify the most parsimonious physical model.

    Validation: Compares the discovered gravity/length ratio (Lg​) against theoretical expectations.

📋 Prerequisites

    MATLAB (R2021a or later recommended)

    Phyphox (Mobile App) to export Gyroscope data in .csv format.

📖 How to Use

    Collect Data: Use a smartphone attached to a pendulum. Record at least 30 seconds of motion using the "Gyroscope" tool in Phyphox.

    Export: Save the data as a CSV (comma-separated values).

    Run Script: Execute the MATLAB script. It will prompt you to select your file.

    Analyze: The script will output a Discovery Report in the command window and generate a verification plot.

📊 Results & Physical Insights

The model evaluates three primary physical components:

    Gravity (sinθ): Used to calculate the "Effective Length" of the pendulum.

    Linear Friction: Damping caused by the pivot point.

    Air Drag: Quadratic damping proportional to ω2.

Example Output
Plaintext

====================================
     SINDy PHYSICAL DISCOVERY       
====================================
Friction (Linear):      -0.0124
Gravity (sin theta):    -22.1450
Air Drag (Quadratic):   -0.0031
------------------------------------
Textbook Expectation:   22.81
Measured Error:         2.91%
Effective Pendulum L:   44.30 cm (Measured: 43cm)
====================================

🛠 Hardware Configuration

    Sensor: Google Pixel 10 (Internal IMU)

    MCU (Future Work): Integration with Nucleo-H743ZI for real-time SINDy execution.

    Pendulum Length: 0.43m (measured from pivot to sensor center).

🎓 Academic Context

Developed as part of a personal validation study within the Analytical Instruments, Measurement and Sensor Technology (AIMS) framework at Hochschule Coburg.

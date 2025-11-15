# Validation of the SimGrid CO2 emissions plugin

This document describes how we validated our plugin to compute the emissions from electricity consumption of machines in SimGrid. We consider static and dynamic values for the input static of grid electricity emissions values (to represent for instance the variability caused by using intermittent renewable sources), and we validate for the following scenarios:

- Machines off;
- Machines idle;
- Performing computations
---

## Inputs
We use homogeneous machines with the following hardware properties: 

| CPU cores used | Power (W) |
|------------------|-----------|
| 1 core           | 125 W     |
| 2 cores          | 150 W     |
| 3 cores          | 175 W     |
| 4 cores          | 200 W     |

| State | Power (W) |
|-------|-----------|
| Idle  | 100 W     |
| Off   | 5 W       |

**Note:** Our plugin extends SimGrid’s energy plugin, which has been validated on both homogeneous and heterogeneous platforms. Although the scenarios below consider only homogeneous machines, the plugin also supports heterogeneous machines.

---
## Carbon-Intensity Data

We consider three grid locations, and use data provided by [ElectricityMap](https://www.electricitymaps.com/) data:

- **USA** —    higher carbon intensity, low share of renewable sources in the electricity grid.
- **Brazil** — intermediate carbon intensity, has a presence of renewables, such as hydroelectric power
- **France** — lower carbon intensity, due to nuclear power


---
# Validation scenarios

All scenarios use a **10‑hour time horizon**.

---

# 1. Static CO₂ Scenarios

## 1.1 Machine Off (USA grid, carbon intensity = 403.65 gCO2/kWh)

- E_J = 5 W  (power when off) * 3600 (seconds in one hour) * 10 (hours in the time horizon) = 180000  J
- E_kWh = 180000 / 3600000 = 0.05 kWh

- **Expected emissions:**  0.05 * 403.65 = **20.1825 g** CO2

---

## 1.2 Machine Idle (France grid, carbon intensity = 33.45 gCO2/kWh

- Power idle = 100 W
- E_J = 100 * 3600 * 10 = 3600000 J
- E_kWh = 1.0 kWh
- **Expected emissions:** 1.0 * 33.45 = **33.45 gCO2**

---

## 1.3 Machine Running Tasks (Brazil grid, carbon intensity = 91.48 gCO2/kWh)

We evaluate running tasks using all the cores in the machine in the following 4 scenarios:
- A : running computations using 1 CPU core for 1 hour
- B : running computations using 2 CPU cores for 2 hours
- C : running computations using 3 CPU cores for 3 hours
- D : running computations using 4 CPU cores for 4 hours

| Scenarios | Cores | Power (W) | Duration (h) | Energy (kWh) | CO₂ (g) |
|---------|-------|-----------|--------------|--------------|---------|
| A       | 1     | 125       | 1            | 0.125        | 11.435  |
| B       | 2     | 150       | 2            | 0.300        | 27.444  |
| C       | 3     | 175       | 3            | 0.525        | 48.027  |
| D       | 4     | 200       | 4            | 0.800        | 73.184  |
| **Total** | —   | —         | 10           | **1.750**    | **160.09** |


---

# 2. Dynamic CO2 Scenarios

In these scenarios, the carbon intensity varies hourly.

---

## 2.1 Machine Off — using hourly carbon intensity from USA 

Power = 5 W  = 0.005 kWh/hour.

| Hour | CI (gCO₂/kWh) | Energy (kWh) | Hourly CO₂ (g) |
|-----:|--------------:|-------------:|---------------:|
| 1 | 453.54 | 0.005 | 2.2677 |
| 2 | 441.48 | 0.005 | 2.2074 |
| 3 | 437.93 | 0.005 | 2.1897 |
| 4 | 437.61 | 0.005 | 2.1881 |
| 5 | 442.29 | 0.005 | 2.2115 |
| 6 | 447.18 | 0.005 | 2.2359 |
| 7 | 452.04 | 0.005 | 2.2602 |
| 8 | 453.96 | 0.005 | 2.2698 |
| 9 | 455.13 | 0.005 | 2.2757 |
| 10 | 457.54 | 0.005 | 2.2877 |
| **Total** | — | **0.05** | **22.3935 gCO₂** |

---

## 2.2 Machine Idle — using hourly carbon intensity from France 

Power = 100 W = 0.1 kWh/hour.

| Hour | CI (gCO₂/kWh) | Energy (kWh) | Hourly CO₂ (g) |
|-----:|--------------:|-------------:|---------------:|
| 1 | 29.09 | 0.1 | 2.909 |
| 2 | 30.08 | 0.1 | 3.008 |
| 3 | 32.33 | 0.1 | 3.233 |
| 4 | 32.96 | 0.1 | 3.296 |
| 5 | 33.00 | 0.1 | 3.300 |
| 6 | 33.41 | 0.1 | 3.341 |
| 7 | 34.52 | 0.1 | 3.452 |
| 8 | 33.63 | 0.1 | 3.363 |
| 9 | 32.17 | 0.1 | 3.217 |
| 10 | 31.95 | 0.1 | 3.195 |
| **Total** | — | **1.0** | **32.314 gCO₂** |

---

## 2.3 Machine Running Tasks — using hourly carbon intensity from  Brazil 

Power consumption varies by active core count (same as Section 1.3).

### Hourly Carbon Intensities
| Hour | CI (gCO₂/kWh) |
|-----:|--------------:|
| 1 | 100.07 |
| 2 | 93.60 |
| 3 | 93.89 |
| 4 | 96.04 |
| 5 | 95.00 |
| 6 | 94.40 |
| 7 | 94.11 |
| 8 | 94.99 |
| 9 | 96.44 |
| 10 | 99.76 |

### Energy & Emissions by Task Segment

| Scenarios | Cores | Hours Covered | Power (W) | Energy (kWh/hour)  | CO₂ (g) Calc |
|---------|-------|---------------|-----------|-------------------|---------------|
| A | 1 | 1 | 125 | 0.125 | 0.125 × 100.07 = 12.50875 |
| B | 2 | 2, 3| 150 | 0.150 | 0.15 × (93.60 + 93.89) = 28.12350 |
| C | 3 | 4, 5, 6  | 175 | 0.175 |  0.175 × (96.04 + 95.00 + 94.40) = 49.95200 |
| D | 4 | 7, 8, 9, 10| 200 | 0.200 |  0.20 × (94.11 + 94.99 + 96.44 + 99.76) = 77.06000 |
| **Total** | — | 10 h | — | **1.750 kWh** |  **167.64425 gCO₂** |

---
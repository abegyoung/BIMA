#!/usr/bin/env python3

import numpy as np
from astropy.time import Time
from astropy.coordinates import (
    SkyCoord,
    EarthLocation,
    FK4,
    get_body_barycentric_posvel,
    solar_system_ephemeris,
)
from astropy.coordinates import LSR
from astropy import units as u
from astropy.constants import c
import argparse

# ============================================================
# PARSER
# ============================================================

parser = argparse.ArgumentParser(description="Look up object coordinates by catalog name.")
parser.add_argument("--target", type=str)
parser.add_argument("--vlsr", type=float)

args = parser.parse_parser_arguments = parser.parse_args()

# ============================================================
# TARGET RA, DEC
# ============================================================
fk4_frame = FK4(equinox="J2000")
target=SkyCoord.from_name(args.target, frame=fk4_frame)

print("\nTARGET")
print("------")
print("ICRS:", target.to_string("hmsdms"))

# Unit vector to source (ICRS Cartesian)
s = target.cartesian.xyz.value
s = s / np.linalg.norm(s)

# ============================================================
# OBSERVATORY
# ============================================================

lat = 32.581122     # Biosphere 2 BIMA 6-meter location
lon = -110.849259
hgt = 1164

location = EarthLocation( lat=lat * u.deg, lon=lon * u.deg, height=hgt * u.m)

# ============================================================
# TIME
# ============================================================

t = Time.now()

print("\nTIME")
print("----")
print("UTC:", t.utc.iso)


# ============================================================
# 1. EARTH ROTATION (TOPOCENTRIC - GEOCENTER)
# ============================================================

# GCRS velocity includes ONLY Earth rotation
v_rot = location.get_gcrs(t).velocity.d_xyz.to(u.m/u.s).value

v_rot_los = np.dot(v_rot, s) / 1000.0
v_rot_mag = np.linalg.norm(v_rot) / 1000.0

print("\n[1] Earth rotation (topocentric)")
print("--------------------------------")
print(f"Speed      : {v_rot_mag:10.6f} km/s")
print(f"LOS        : {v_rot_los:10.6f} km/s")


# ============================================================
# 2. EARTH ORBITAL MOTION (GEOCENTER → BARYCENTER)
# ============================================================

with solar_system_ephemeris.set("builtin"):
    earth_pv = get_body_barycentric_posvel("earth", t)

v_earth = earth_pv[1].xyz.to(u.m/u.s).value

v_earth_los = np.dot(v_earth, s) / 1000.0
v_earth_mag = np.linalg.norm(v_earth) / 1000.0

print("\n[2] Earth barycentric orbital motion")
print("-------------------------------------")
print(f"Speed      : {v_earth_mag:10.6f} km/s")
print(f"LOS        : {v_earth_los:10.6f} km/s")

# ============================================================
# 3. SOLAR MOTION RELATIVE TO LSR (ROBUST VERSION)
# ============================================================

# Standard IAU "kinematic LSR" solar motion (common radio choice):
# (Schönrich et al. / old standard radio definition is similar scale)

v_sun_lsr = np.array([11.1, 12.24, 7.25])  # km/s (U,V,W)

# Convert to ICRS-like basis requires axis alignment assumption:
# Astropy ICRS:
# +x = RA=0, Dec=0
# +y = RA=6h
# +z = Dec=+90

# For practical radio use, we assume already in ICRS-equivalent axes
v_sun = v_sun_lsr

v_sun_los = np.dot(v_sun, s)
v_sun_mag = np.linalg.norm(v_sun)

print("\n[3] Solar motion relative to LSR")
print("--------------------------------")
print(f"Speed      : {v_sun_mag:10.6f} km/s")
print(f"LOS        : {v_sun_los:10.6f} km/s")

# ============================================================
# 4. TOTAL OBSERVER VELOCITY (ALL COMPONENTS)
# ============================================================

v_obs_los = v_sun_los + v_earth_los + v_rot_los

print("\n[4] TOTAL OBSERVER LOS VELOCITY")
print("-------------------------------")
print(f"v_obs_LOS  : {v_obs_los:10.6f} km/s")


# ============================================================
# 5. OBJECT VELOCITY (LSR)
# ============================================================

v_cloud = args.vlsr  # km/s

print("\n[5] Object velocity (LSR)")
print("--------------------------------")
print(f"v_cloud    : {v_cloud:10.6f} km/s")


# ============================================================
# 6. TOPOCENTRIC APPARENT VELOCITY
# ============================================================

v_topo = v_cloud - v_obs_los

print("\n[6] Topocentric velocity")
print("------------------------")
print(f"v_topo     : {v_topo:10.6f} km/s")


# ============================================================
# 7. DOPPLER SHIFT OF CO LINE
# ============================================================

f0 = 115.271203e9  # Hz

v_ms = v_topo * 1000.0

f_obs = f0 * (1.0 - v_ms / c.value)

df = f_obs - f0

print("\n[7] CO J=1→0 frequency shift")
print("----------------------------")
print(f"Rest freq  : {f0/1e9:.6f} GHz")
print(f"Shift      : {df/1e6:10.3f} MHz")
print(f"Observed   : {f_obs/1e9:.6f} GHz")
print(f"LO freq    : {(f_obs-7450000000)/1e9:.6f} GHz")

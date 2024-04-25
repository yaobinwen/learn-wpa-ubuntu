# Learn WPA

## Overview

This repository contains the versions of [wpa](https://launchpad.net/ubuntu/+source/wpa) source files that I'm interested in and the notes I made for learning purpose. Because I mainly work on Ubuntu, the source code all comes from the `wpa` package on Launchpad. Therefore, the code in this repository belongs to the original authors and should be used under the same license. See [their license on Launchpad](https://git.launchpad.net/ubuntu/+source/wpa/tree/COPYING) or [the license on Debian](https://salsa.debian.org/debian/wpa/-/blob/debian/unstable/COPYING?ref_type=heads).

Each version is in its own sub-folder. My notes are all marked with `NOTE(ywen)`. The log messages I added are prefixed with `[ywen]`.

## Versions

- `2%2.6-15ubuntu2`: The version used on Ubuntu 18.04 (with patches applied). Corresponding tag: [`applied/2%2.6-15ubuntu2`](https://git.launchpad.net/ubuntu/+source/wpa/tag/?h=applied/2%252.6-15ubuntu2).
  - I have to replace `:` right after the epoch version with `%` (as the git tag did) because `make` doesn't seem to like `:` in the file paths. It would report "multiple target patterns" errors.

## How to build

See the section "Building and installing" in `wpa_supplicant/README` for detailed instructions. In general:
- Install the build dependencies. See [`install-deps.sh`](./install-deps.sh).
- Create `wpa_supplicant/.config` to select the features to be built.
- Inside `wpa_supplicant`, just run `make`.

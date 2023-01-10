# Flipper Iambic Morse Key

An iambic morse key for the Flipper Zero.

## Building Locally

To build locally:

```sh
make
```

Your `fap` (Flipper Application Package) can be found in `build/dist/f7-D/apps/apps/<category>/<appid>.fap`.

To clean the build folder, you can also:

```sh
make clean
```

## Building as a GitHub Action

This repository also contains a GitHub workflow to build the `fap` on push to the `main` branch and on pull request. The compiled `fap` file is made available as an artifact of the build workflow.

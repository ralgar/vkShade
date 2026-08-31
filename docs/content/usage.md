# Usage

Enable vkShade by setting the environment variable.

## Standard

When using the terminal or an application (.desktop) file, execute:

```
ENABLE_VKSHADE=1 yourgame
```

## Lutris

With Lutris, follow these steps below:

1. Right click on a game, and press `configure`.
1. Go to the `System options` tab and scroll down to `Environment variables`.
1. Press on `Add`, and add `ENABLE_VKSHADE` under `Key`, and add `1` under `Value`.

## Steam

With Steam, edit your game's launch options and add:
```
ENABLE_VKSHADE=1 %command%
```

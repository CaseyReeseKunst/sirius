# sirius
Sirius is a program for playing the game of reversi. The program includes an AI opponent which plays at a very challenging level. Its strength can therefore be adjusted in several ways to give you a suitable opponent.

The AI opponent uses a plain alpha-beta search with hashing to figure out which move to make. To be able to tell a good position from a bad one, it uses a pattern based evaluation function. The pattern used is the 9 discs surrounding each corner and the 8 discs creating the edge of the board. The evaluation function also takes mobility, potential mobility and parity into count. For the initial 9 moves the AI opponent optionally uses a simple opening book.

Siruis was written in C by Henrik Öhman. It uses the GTK GUI toolkit. This software is distributed with GNU General Public License, version 2.

This software package no longer has a maintainer. Fork it to maintain it or develop it further.

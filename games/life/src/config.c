#include "config.h"

const LifePreset LIFE_PRESETS[MAX_PRESETS] = {
  {
    "Glider",
    "Spaceship (Diagonal)",
    3, 3,
    {
      ".O.",
      "..O",
      "OOO"
    }
  },
  {
    "LWSS",
    "Spaceship (Horizontal)",
    5, 4,
    {
      ".O..O",
      "....O",
      "O...O",
      ".OOOO"
    }
  },
  {
    "Blinker",
    "Oscillator (Period 2)",
    3, 3,
    {
      ".O.",
      ".O.",
      ".O."
    }
  },
  {
    "Toad",
    "Oscillator (Period 2)",
    4, 2,
    {
      ".OOO",
      "OOO."
    }
  },
  {
    "Beacon",
    "Oscillator (Period 2)",
    4, 4,
    {
      "OO..",
      "OO..",
      "..OO",
      "..OO"
    }
  },
  {
    "Pulsar",
    "Oscillator (Period 3)",
    13, 13,
    {
      "..OOO...OOO..",
      ".............",
      "O....O.O....O",
      "O....O.O....O",
      "O....O.O....O",
      "..OOO...OOO..",
      ".............",
      "..OOO...OOO..",
      "O....O.O....O",
      "O....O.O....O",
      "O....O.O....O",
      ".............",
      "..OOO...OOO.."
    }
  },
  {
    "Pentadecathlon",
    "Oscillator (Period 15)",
    10, 3,
    {
      "..O....O..",
      "OO.OOOO.OO",
      "..O....O.."
    }
  },
  {
    "R-pentomino",
    "Methuselah (Chaotic)",
    3, 3,
    {
      ".OO",
      "OO.",
      ".O."
    }
  },
  {
    "Acorn",
    "Methuselah (5200+ Gen)",
    7, 3,
    {
      ".O.....",
      "...O...",
      "OO..OOO"
    }
  },
  {
    "Beehive & Loaf",
    "Still Lifes (Stable)",
    8, 4,
    {
      ".OO...OO",
      "O..O.O..O",
      ".OO...O.O",
      ".......O."
    }
  }
};

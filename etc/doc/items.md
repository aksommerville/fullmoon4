# Full Moon Items

### Broom

Hold button to hover.
When hovering:
 - Pass over holes.
 - Move faster.
 - Slower deceleration.
 
Ensure that we remaining hovering if the button is released while over a hole.
User is allowed to equip a different item in this state too.
As soon as we are over dry land, the hover ends.

Automatic motion due to injury should also run faster while hovering, and throw further.

### Wand

Hold, and press dpad cardinals to encode a spell.
Spell is committed when the button is released.

There will of course be a spell length limit. Ensure that if the user exceeds it, it does not match.
(even if the valid-length prefix or suffix of what she entered is a valid spell).

I think 5 is fair for the shortest spell, and 8 for the longest?
That yields a space of 87040 possible spells, in which we'll define maybe 10.

Spells:
 - Teleport home.
 - A few other teleport points.
 - Open doors.
 - Rain.
 - Slow motion.
 - Wind.

### Violin

Same idea as the wand, but rhythm matters.

For encoding purposes, we'll display the last 16 beats or so, and whenever that part matches a song, it commits instantly.

16 beats yields a space of 152587890625 possible songs, surely wide enough.
We'll probably stick to alternate beats for the most part, which is still wide with 390625 songs.

TODO There must be some thematic property that distinguishes spells from songs.

Songs:
 - Put animals to sleep.
 - Bring statues to life (moving back and forth so you can walk past).
 - Make flowers bloom.
 - Song of Revelations: Cause garbled writing to decrypt itself.

### Compass

Button does nothing.
While equipped, an arrow orbits the hero.
If a secret is in range, the arrow moves slower when pointing in its direction.
Max range, say 2 screen widths?

I think it's OK to update the secret position only at screen transitions, or when it's collected.

There will also be tricks, eg a row of tiles where when you stand on them the compass points in a cardinal direction.
Teach the user a spell that way.

Secrets:
 - Buried treasure.
 - Hidden door.
 - Unopened treasure chests in plain sight.
 - Written spells and songs. (lowest priority, because they aren't collectible)
 - Pick one of several doors. The compass shows you which one is not a trap.

### Pitcher

Fill with various liquids.
The pitcher is either empty, or contains one unit of one liquid.

Pour liquids:
 - On planted seeds to make them grow.
 - On fires to extinguish them. (or if it's Honey, to expand them). (because Honey is highly flammable, because I said so).
 - On thirsty animals to sate them.
 
Liquids:
 - Water. From rivers etc. Plants will yield seed. Generally behaves the same as the rain spell. Don't let water do anything too interesting.
 - Milk. From cows. Plants yield cheese.
 - Honey. From hives. Plants yield coins.
 - Sap. From trees (abundant). Plants yield matches.

### Seed

Quantity.

Place on soil disturbed by the shovel to plant.
A planted seed must first be watered, then it sprouts and after some time, blooms.
A bloomed plant has an item you can collect: seed, nitro, match, corn, coin.
Seeds on the ground will attract a crow, which after eating will lead you somewhere.

### Coin

Quantity.

Trade in shops. Are there going to be shops?

Pay off the occasional Toll Troll.

Throw like a weapon.

### Match

Quantity.

Once used, a fire lights in front of you, and dark rooms are illuminated, for a fixed time.
The illumination remains for its scheduled time even if you equip something else.

Note that we skipped the obvious "fire spell", in favor of this depletable asset.

### Umbrella

Hold button to deploy in front of you.

While deployed, the hero does not change direction, and is shielded from missiles in that direction.

Not changing direction is important. I want something like "don't look at Medusa or you turn to stone".

### Feather

Hold button to waggle it in front of you.

Mostly it will influence blocks. Tickling a block may trigger some behavior from it.

- pushblock: Follow me, one direction only.
- alphablock_alpha: Tickle each edge in a clockwise pattern to move by one cell.
- alphablock_gamma: Tickle LRL, disappear.
- alphablock_lambda: Summon from a distance.
- alphablock_mu: 3A 1B 1A, move by one cell.

I want the feather to be the first thing you get, and the final play to kill the werewolf.
"A story that begins with a feather, ends with a feather."

### Shovel

Just like Link's Awakening. Dig up one cell of earth.

There may be treasure underground.
Shovel-disturbed soil can have a seed planted in it.
Occasionally, digging somewhere reveals a stairway.

While equipped, highlight the cell that will be affected. It won't be obvious no matter what.

### Chalk

Press button while near a south-facing surface, and facing it, to enter sketch mode.

In sketch mode, you get a 3x3 grid of dots which you can selectively connect or erase.

A certain number of sketches will persist. 20 bits each, plus whatever we need for the position.

Chalk tricks:
 - A word pre-written on the wall that you must change to some other word to make an obstacle move.
 - Complete the pattern. eg card suits, sequential letters or numbers, one is initially missing.
 - Encode answer to counting challenges. "How many ghosts in the dungeon?"
 - Summon a ghost or demon. (TODO to what end?)
 - A sentinel somehow expresses he wants something, say a lollipop. Draw one on the wall and he's drawn to it.
 - Draw a door, then cast the Spell of Opening and it opens. Takes you usually to the same neutral stub room, but in some places, magic!

### Bell

Press button and it rings.

Scares away animals, and draws attention of humans.

TODO Needs some tricks.

### Cheese

Use to temporarily increase walking speed.

### Membership Hat

Passively changes Dot's appearance.
A certain class of monster will attack you if not wearing the hat.
(and since it's an item, you can't do anything else while wearing)

### Snow Globe

Wield like the wand: Hold A, then tap the dpad.
Wielding in one direction causes an earthquake that shuffles all moveable sprites the opposite direction.

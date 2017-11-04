# Licensing FAQ

## Why LGPL v3.0?

To protect myself.

Normally I'm very liberal with the usage of my code, I tend to prefer licensing under MIT or BSD or something. However, I have dedicated so
much of my life to writing Simulant that the fear of someone coming along, packaging it up and selling it is too great. 

## But... why require patches under MIT and LGPL?

Because I want to be able to change my mind in future. 

If I accept patches under LGPL, that means I have to get agreement from all contributors to change it later. If all patches are submitted
under MIT, then that allows me to later re-licence the whole engine under MIT. 

## Doesn't LGPL limit what I can do?

Yes, probably. LGPL is a 'viral' license.

If you statically link to the Simulant library, then you must release the source code for your application under a compatible license. However, if you dynamically link to the Simulant library (and make no changes to it) then you're fine.

If you are writing a Dreamcast application, then this means you must release the source code as you will need to statically link the library.

## But... I want to use Simulant for a Dreamcast game, but don't want to open-source it

That's cool, contact me, we can discuss it. But don't be suprised if I ask for moneyz if you're planning to sell for a profit :)


LSI is still in its infancy, currently only at a 0.1 release. The time to
release was dramatic due to an absolute requirement for non-broken Steam
to accompany the Solus 1.2 release

Going forward, there are more items that need implementing to start shaving
more rough edges off of Linux gaming.

Immediate LSI worklist:

 - [x] Add configurable shim to replace /usr/bin/steam, relying on shadowed launcher
 - [x] Add super-basic UI to configure said shim
 - [x] Shove out a 0.1 for Solus 1.2
 - [ ] Replace existing /usr/bin/steam entirely and decouple the tight runtime logic
       that is currently enforced.
 - [ ] Remove requirement for steam.sh $bindir/steam entirely - allowing parallel clients.
       LSI becomes more self contained and no longer needs to prop up Steam, but replaces
       much of the support tools.
 - [ ] Replace the existing Steam "run.sh" style launch facilities
 - [ ] Get to a point that current ubuntu12_32 runtime is no longer required or
       wanted.

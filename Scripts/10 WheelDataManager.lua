-- like a singleton
-- this manages all the data for the musicwheel
-- the purpose of it is to essentially cache wheel related things and sorts
-- we don't have to keep resorting the wheel every time we open a pack if this is correct
-- etc

WHEELDATA = {}

-- a function for sorting songs by title according to how the game does it in C++
function WHEELDATA.CompareSongsByTitle(a, b)
    return SongUtil.SongTitleComparator(a, b)
end
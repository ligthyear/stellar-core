#pragma once

// Copyright 2014 Stellar Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "overlay/StellarXDR.h"
#include "bucket/LedgerCmp.h"
#include "util/NonCopyable.h"

/*
Frame
Parent of AccountFrame, TrustFrame, OfferFrame

These just hold the xdr LedgerEntry objects and have some associated functions
*/

namespace stellar
{
class Database;
class LedgerDelta;

class EntryFrame : public NonMovableOrCopyable
{
  protected:
    mutable bool mKeyCalculated;
    mutable LedgerKey mKey;
    void
    clearCached()
    {
        mKeyCalculated = false;
    }

  public:
    typedef std::shared_ptr<EntryFrame> pointer;

    LedgerEntry mEntry;

    EntryFrame(LedgerEntryType type);
    EntryFrame(LedgerEntry const& from);

    static pointer FromXDR(LedgerEntry const& from);
    static pointer storeLoad(LedgerKey const& key, Database& db);

    // Static helpers for working with the DB LedgerEntry cache.
    static void flushCachedEntry(LedgerKey const& key, Database& db);
    static bool cachedEntryExists(LedgerKey const& key, Database& db);
    static std::shared_ptr<LedgerEntry const> getCachedEntry(
        LedgerKey const& key, Database& db);
    static void putCachedEntry(LedgerKey const& key,
                               std::shared_ptr<LedgerEntry const> p,
                               Database& db);

    // Member helpers that call cache flush/put for self.
    void flushCachedEntry(Database& db) const;
    void putCachedEntry(Database& db) const;

    static void checkAgainstDatabase(LedgerEntry const& entry,
                                     Database& db);

    virtual EntryFrame::pointer copy() const = 0;

    LedgerKey const& getKey() const;
    virtual void storeDelete(LedgerDelta& delta, Database& db) const = 0;
    virtual void storeChange(LedgerDelta& delta, Database& db) const = 0;
    virtual void storeAdd(LedgerDelta& delta, Database& db) const = 0;

    void storeAddOrChange(LedgerDelta& delta, Database& db) const;
    static bool exists(Database& db, LedgerKey const& key);
    static void storeDelete(LedgerDelta& delta, Database& db,
                            LedgerKey const& key);
};

// static helper for getting a LedgerKey from a LedgerEntry.
LedgerKey LedgerEntryKey(LedgerEntry const& e);
}

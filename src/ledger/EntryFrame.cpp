// Copyright 2014 Stellar Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "ledger/EntryFrame.h"
#include "LedgerManager.h"
#include "ledger/AccountFrame.h"
#include "ledger/OfferFrame.h"
#include "ledger/TrustFrame.h"
#include "xdrpp/printer.h"
#include "xdrpp/marshal.h"
#include "crypto/Hex.h"
#include "database/Database.h"

namespace stellar
{
using xdr::operator==;

EntryFrame::pointer
EntryFrame::FromXDR(LedgerEntry const& from)
{
    EntryFrame::pointer res;

    switch (from.type())
    {
    case ACCOUNT:
        res = std::make_shared<AccountFrame>(from);
        break;
    case TRUSTLINE:
        res = std::make_shared<TrustFrame>(from);
        break;
    case OFFER:
        res = std::make_shared<OfferFrame>(from);
        break;
    }
    return res;
}

EntryFrame::pointer
EntryFrame::storeLoad(LedgerKey const& key, Database& db)
{
    EntryFrame::pointer res;

    switch (key.type())
    {
    case ACCOUNT:
        res = std::static_pointer_cast<EntryFrame>(
            AccountFrame::loadAccount(key.account().accountID, db));
        break;
    case TRUSTLINE:
    {
        auto const& tl = key.trustLine();
        res = std::static_pointer_cast<EntryFrame>(
            TrustFrame::loadTrustLine(tl.accountID, tl.asset, db));
    }
    break;
    case OFFER:
    {
        auto const& off = key.offer();
        res = std::static_pointer_cast<EntryFrame>(
            OfferFrame::loadOffer(off.sellerID, off.offerID, db));
    }
    break;
    }
    return res;
}

void
EntryFrame::flushCachedEntry(LedgerKey const& key, Database& db)
{
    auto s = binToHex(xdr::xdr_to_opaque(key));
    db.getEntryCache().erase_if_exists(s);
}


bool
EntryFrame::cachedEntryExists(LedgerKey const& key, Database& db)
{
    auto s = binToHex(xdr::xdr_to_opaque(key));
    return db.getEntryCache().exists(s);
}

std::shared_ptr<LedgerEntry const>
EntryFrame::getCachedEntry(LedgerKey const& key, Database& db)
{
    auto s = binToHex(xdr::xdr_to_opaque(key));
    return db.getEntryCache().get(s);
}

void
EntryFrame::putCachedEntry(LedgerKey const& key,
                           std::shared_ptr<LedgerEntry const> p, Database& db)
{
    auto s = binToHex(xdr::xdr_to_opaque(key));
    db.getEntryCache().put(s, p);
}

void
EntryFrame::flushCachedEntry(Database& db) const
{
    flushCachedEntry(getKey(), db);
}

void
EntryFrame::putCachedEntry(Database& db) const
{
    putCachedEntry(getKey(), std::make_shared<LedgerEntry const>(mEntry), db);
}

void
EntryFrame::checkAgainstDatabase(LedgerEntry const& entry, Database& db)
{
    auto key = LedgerEntryKey(entry);
    flushCachedEntry(key, db);
    auto const& fromDb = EntryFrame::storeLoad(key, db);
    if (!(fromDb->mEntry == entry))
    {
        std::string s;
        s = "Inconsistent state between objects: ";
        s += xdr::xdr_to_string(fromDb->mEntry, "db");
        s += xdr::xdr_to_string(entry, "live");
        throw std::runtime_error(s);
    }
}

EntryFrame::EntryFrame(LedgerEntryType type)
    : mKeyCalculated(false), mEntry(type)
{
}

EntryFrame::EntryFrame(LedgerEntry const& from)
    : mKeyCalculated(false), mEntry(from)
{
}

LedgerKey const&
EntryFrame::getKey() const
{
    if (!mKeyCalculated)
    {
        mKey = LedgerEntryKey(mEntry);
        mKeyCalculated = true;
    }
    return mKey;
}

void
EntryFrame::storeAddOrChange(LedgerDelta& delta, Database& db) const
{
    if (exists(db, getKey()))
    {
        storeChange(delta, db);
    }
    else
    {
        storeAdd(delta, db);
    }
}

bool
EntryFrame::exists(Database& db, LedgerKey const& key)
{
    switch (key.type())
    {
    case ACCOUNT:
        return AccountFrame::exists(db, key);
    case TRUSTLINE:
        return TrustFrame::exists(db, key);
    case OFFER:
        return OfferFrame::exists(db, key);
    default:
        abort();
    }
}

void
EntryFrame::storeDelete(LedgerDelta& delta, Database& db, LedgerKey const& key)
{
    switch (key.type())
    {
    case ACCOUNT:
        AccountFrame::storeDelete(delta, db, key);
        break;
    case TRUSTLINE:
        TrustFrame::storeDelete(delta, db, key);
        break;
    case OFFER:
        OfferFrame::storeDelete(delta, db, key);
        break;
    }
}

LedgerKey
LedgerEntryKey(LedgerEntry const& e)
{
    LedgerKey k;
    switch (e.type())
    {

    case ACCOUNT:
        k.type(ACCOUNT);
        k.account().accountID = e.account().accountID;
        break;

    case TRUSTLINE:
        k.type(TRUSTLINE);
        k.trustLine().accountID = e.trustLine().accountID;
        k.trustLine().asset = e.trustLine().asset;
        break;

    case OFFER:
        k.type(OFFER);
        k.offer().sellerID = e.offer().sellerID;
        k.offer().offerID = e.offer().offerID;
        break;
    }
    return k;
}
}

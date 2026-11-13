#include <gtest/gtest.h>
#include "../../src/Infrastructure/Persistence/MemorySession.h"

using namespace ReservationDomain;
using namespace ReservationInfrastructure;

TEST(MemorySessionTests, SaveNewRequest)
{
    MemorySession session;
    ReservationRequest request(RequestId("REQ-100"), ProductId("PROD-A"), DepartmentId("DEPT-1"), Quantity(5));

    session.save(request);
    session.commit();

    auto loaded = session.findById(RequestId("REQ-100"));
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(session.getRevision(RequestId("REQ-100")), 1);
}

TEST(MemorySessionTests, LoadSavedRequest)
{
    MemorySession session;
    ReservationRequest original(RequestId("REQ-101"), ProductId("PROD-B"), DepartmentId("DEPT-2"), Quantity(3));
    session.save(original);
    session.commit();

    auto loaded = session.findById(RequestId("REQ-101"));
    ASSERT_TRUE(loaded.has_value());
    EXPECT_NE(&original, &(*loaded));
    EXPECT_EQ(loaded->getRequestId().getValue(), "REQ-101");
}

TEST(MemorySessionTests, RestoreReservedStatus)
{
    MemorySession session;
    session.seed({ "REQ-102", "PROD-C", "DEPT-1", 10, ReservationStatus::Reserved, "", 2 });

    auto loaded = session.findById(RequestId("REQ-102"));
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->getStatus(), ReservationStatus::Reserved);
    EXPECT_TRUE(loaded->getRejectionReason().empty());
}

TEST(MemorySessionTests, RestoreRejectedStatusWithReason)
{
    MemorySession session;
    std::string reason = "Недостатня кількість товару. Запитано: 10, Доступно: 2";
    session.seed({ "REQ-103", "PROD-D", "DEPT-3", 10, ReservationStatus::Rejected, reason, 1 });

    auto loaded = session.findById(RequestId("REQ-103"));
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->getStatus(), ReservationStatus::Rejected);
    EXPECT_EQ(loaded->getRejectionReason(), reason);
}

TEST(MemorySessionTests, SavePostProcessingChanges)
{
    MemorySession session;
    ReservationRequest request(RequestId("REQ-104"), ProductId("PROD-E"), DepartmentId("DEPT-1"), Quantity(5));
    session.save(request);
    session.commit();

    auto loaded = session.findById(RequestId("REQ-104")).value();
    loaded.process(AvailableQuantity(2));

    session.save(loaded);
    session.commit();

    auto updated = session.findById(RequestId("REQ-104")).value();
    EXPECT_EQ(updated.getStatus(), ReservationStatus::Rejected);
    EXPECT_EQ(session.getRevision(RequestId("REQ-104")), 2);
}

TEST(MemorySessionTests, VersionConflictDetection)
{
    auto sharedStorage = std::make_shared<MemoryStorage>();
    MemorySession setupSession(sharedStorage);
    setupSession.seed({ "REQ-105", "PROD-F", "DEPT-1", 5, ReservationStatus::Submitted, "", 1 });

    MemorySession sessionUser1(sharedStorage);
    MemorySession sessionUser2(sharedStorage);

    auto reqUser1 = sessionUser1.findById(RequestId("REQ-105")).value();
    auto reqUser2 = sessionUser2.findById(RequestId("REQ-105")).value();

    reqUser1.process(AvailableQuantity(10));
    sessionUser1.save(reqUser1);
    sessionUser1.commit();

    reqUser2.process(AvailableQuantity(0));
    sessionUser2.save(reqUser2);

    EXPECT_THROW(sessionUser2.commit(), std::runtime_error);

    MemorySession verifySession(sharedStorage);
    auto finalReq = verifySession.findById(RequestId("REQ-105")).value();
    EXPECT_EQ(finalReq.getStatus(), ReservationStatus::Reserved);
}

TEST(MemorySessionTests, RollbackBeforeCommit)
{
    MemorySession session;
    ReservationRequest request(RequestId("REQ-106"), ProductId("PROD-G"), DepartmentId("DEPT-1"), Quantity(4));

    session.save(request);
    session.rollback();

    auto loaded = session.findById(RequestId("REQ-106"));
    EXPECT_FALSE(loaded.has_value());
}
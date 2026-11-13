#include <gtest/gtest.h>
#include "../src/Model/ReservationRequest.h"
#include "../src/Application/Services/ReservationService.h"
#include <stdexcept>
#include <string>
#include <optional>

using namespace ReservationDomain;
using namespace ReservationApplication;

namespace
{
    ReservationRequest createRequest(int requestedQuantity = 10)
    {
        return ReservationRequest(
            RequestId("REQ-001"),
            ProductId("PROD-001"),
            DepartmentId("DEPT-001"),
            Quantity(requestedQuantity));
    }


    class FakeReservationRepository : public IReservationRepository
    {
    public:
        std::optional<ReservationRequest> storedRequest;
        bool saveCalled = false;
        bool failSave = false;

        std::optional<ReservationRequest> findById(
            const RequestId &requestId) override
        {
            if (!storedRequest.has_value())
            {
                return std::nullopt;
            }

            if (!(storedRequest->getRequestId() == requestId))
            {
                return std::nullopt;
            }

            return storedRequest;
        }

        void save(const ReservationRequest &request) override
        {
            saveCalled = true;

            if (failSave)
            {
                throw std::runtime_error("Save failed");
            }

            storedRequest = request;
        }
    };

    class FakeUnitOfWork : public IUnitOfWork
    {
    public:
        bool commitCalled = false;
        bool rollbackCalled = false;
        bool failCommit = false;

        void commit() override
        {
            commitCalled = true;

            if (failCommit)
            {
                throw std::runtime_error("Commit failed");
            }
        }

        void rollback() override
        {
            rollbackCalled = true;
        }
    };

    class FakeStockAvailability : public IStockAvailabilityPort
    {
    public:
        int availableQuantity = 10;
        bool fail = false;

        AvailableQuantity getAvailableQuantity(
            const ProductId &productId) override
        {
            if (fail)
            {
                throw std::runtime_error("Stock service unavailable");
            }

            return AvailableQuantity(availableQuantity);
        }
    };

    ReservationRequest createApplicationRequest()
    {
        return ReservationRequest(
            RequestId("REQ-APP-001"),
            ProductId("PROD-APP-001"),
            DepartmentId("DEPT-APP-001"),
            Quantity(5));
    }
}


TEST(ReservationDomainTests, RequestCreation)
{
    ReservationRequest request = createRequest();
    EXPECT_EQ(request.getStatus(), ReservationStatus::Submitted);
}

TEST(ReservationDomainTests, SuccessfulReservation)
{
    ReservationRequest request = createRequest(10);
    std::optional<Reservation> result = request.process(AvailableQuantity(15));

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(request.getStatus(), ReservationStatus::Reserved);
    EXPECT_EQ(result->requestId.getValue(), "REQ-001");
    EXPECT_EQ(result->productId.getValue(), "PROD-001");
    EXPECT_EQ(result->departmentId.getValue(), "DEPT-001");
    EXPECT_EQ(result->quantity.getValue(), 10);
}

TEST(ReservationDomainTests, ReservationWithEqualQuantity)
{
    ReservationRequest request = createRequest(10);
    std::optional<Reservation> result = request.process(AvailableQuantity(10));

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(request.getStatus(), ReservationStatus::Reserved);
    EXPECT_EQ(result->quantity.getValue(), 10);
}

TEST(ReservationDomainTests, InsufficientStock)
{
    ReservationRequest request = createRequest(10);
    std::optional<Reservation> result = request.process(AvailableQuantity(5));

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(request.getStatus(), ReservationStatus::Rejected);
    EXPECT_EQ(
        request.getRejectionReason(),
        "Недостатня кількість товару. Запитано: 10, Доступно: 5");
}

TEST(ReservationDomainTests, ZeroAvailableStock)
{
    ReservationRequest request = createRequest(1);
    std::optional<Reservation> result = request.process(AvailableQuantity(0));

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(request.getStatus(), ReservationStatus::Rejected);
    EXPECT_EQ(
        request.getRejectionReason(),
        "Недостатня кількість товару. Запитано: 1, Доступно: 0");
}

TEST(ReservationDomainTests, InvalidRequestedQuantity)
{
    EXPECT_THROW(Quantity(0), std::invalid_argument);
}

TEST(ReservationDomainTests, NegativeAvailableQuantity)
{
    EXPECT_THROW(AvailableQuantity(-1), std::invalid_argument);
}

TEST(ReservationDomainTests, EmptyRequestId)
{
    EXPECT_THROW(RequestId(""), std::invalid_argument);
}

TEST(ReservationDomainTests, RepeatedProcessing)
{
    ReservationRequest request = createRequest(10);
    request.process(AvailableQuantity(15));

    EXPECT_THROW(request.process(AvailableQuantity(15)), std::logic_error);
}

TEST(ReservationDomainTests, EntityIdentity)
{
    ReservationRequest request1(
        RequestId("REQ-001"),
        ProductId("PROD-001"),
        DepartmentId("DEPT-001"),
        Quantity(10));

    ReservationRequest request2(
        RequestId("REQ-002"),
        ProductId("PROD-001"),
        DepartmentId("DEPT-001"),
        Quantity(10));

    EXPECT_FALSE(request1.getRequestId() == request2.getRequestId());

    request1.process(AvailableQuantity(15));
    EXPECT_TRUE(request1.getRequestId() == RequestId("REQ-001"));
}

TEST(ReservationDomainTests, ValueObjectEquality)
{
    Quantity quantity1(10);
    Quantity quantity2(10);
    Quantity quantity3(15);

    EXPECT_TRUE(quantity1 == quantity2);
    EXPECT_FALSE(quantity1 == quantity3);

    RequestId id1("REQ-001");
    RequestId id2("REQ-001");
    RequestId id3("REQ-002");

    EXPECT_TRUE(id1 == id2);
    EXPECT_FALSE(id1 == id3);
}

TEST(ReservationDomainTests, StateUnchangedAfterFailedProcessing)
{
    ReservationRequest request = createRequest(10);
    request.process(AvailableQuantity(15));

    ReservationStatus previousStatus = request.getStatus();
    std::string previousReason = request.getRejectionReason();

    EXPECT_THROW(request.process(AvailableQuantity(15)), std::logic_error);
    EXPECT_EQ(request.getStatus(), previousStatus);
    EXPECT_EQ(request.getRejectionReason(), previousReason);
}

// =========================================================
// 2. ТЕСТИ ВІДНОВЛЕННЯ АГРЕГАТУ (ЛР 4 Анастасія — 4 тести)
// =========================================================

TEST(ReservationRestorationTests, RestoreSubmitted)
{
    ReservationRequest request = ReservationRequest::restore(
        RequestId("REQ-001"),
        ProductId("PROD-001"),
        DepartmentId("DEPT-001"),
        Quantity(10),
        ReservationStatus::Submitted,
        "");

    EXPECT_EQ(request.getStatus(), ReservationStatus::Submitted);
    EXPECT_TRUE(request.getRejectionReason().empty());
    EXPECT_EQ(request.getRequestedQuantity().getValue(), 10);
}

TEST(ReservationRestorationTests, RestoreReserved)
{
    ReservationRequest request = ReservationRequest::restore(
        RequestId("REQ-002"),
        ProductId("PROD-001"),
        DepartmentId("DEPT-001"),
        Quantity(10),
        ReservationStatus::Reserved,
        "");

    EXPECT_EQ(request.getStatus(), ReservationStatus::Reserved);
    EXPECT_THROW(request.process(AvailableQuantity(15)), std::logic_error);
}

TEST(ReservationRestorationTests, RestoreRejected)
{
    const std::string reason =
        "Недостатня кількість товару. Запитано: 10, Доступно: 5";

    ReservationRequest request = ReservationRequest::restore(
        RequestId("REQ-003"),
        ProductId("PROD-001"),
        DepartmentId("DEPT-001"),
        Quantity(10),
        ReservationStatus::Rejected,
        reason);

    EXPECT_EQ(request.getStatus(), ReservationStatus::Rejected);
    EXPECT_EQ(request.getRejectionReason(), reason);
    EXPECT_THROW(request.process(AvailableQuantity(15)), std::logic_error);
}

TEST(ReservationRestorationTests, RestoreInvalidStates)
{
    EXPECT_THROW(
        ReservationRequest::restore(
            RequestId("REQ-004"),
            ProductId("PROD-001"),
            DepartmentId("DEPT-001"),
            Quantity(10),
            ReservationStatus::Rejected,
            ""),
        std::invalid_argument);

    EXPECT_THROW(
        ReservationRequest::restore(
            RequestId("REQ-005"),
            ProductId("PROD-001"),
            DepartmentId("DEPT-001"),
            Quantity(10),
            ReservationStatus::Submitted,
            "Some reason"),
        std::invalid_argument);

    EXPECT_THROW(
        ReservationRequest::restore(
            RequestId("REQ-006"),
            ProductId("PROD-001"),
            DepartmentId("DEPT-001"),
            Quantity(10),
            ReservationStatus::Reserved,
            "Some reason"),
        std::invalid_argument);

    EXPECT_THROW(
        ReservationRequest::restore(
            RequestId("REQ-007"),
            ProductId("PROD-001"),
            DepartmentId("DEPT-001"),
            Quantity(10),
            static_cast<ReservationStatus>(999),
            ""),
        std::invalid_argument);
}


TEST(ReservationApplicationTests, SuccessfulReservation)
{
    FakeReservationRepository repository;
    repository.storedRequest = createApplicationRequest();

    FakeUnitOfWork unitOfWork;
    FakeStockAvailability stock;
    stock.availableQuantity = 10;

    ReservationService service(repository, unitOfWork, stock);
    auto result = service.processRequest(RequestId("REQ-APP-001"));

    EXPECT_EQ(result.status, ReservationStatus::Reserved);
    EXPECT_TRUE(result.reservation.has_value());
    EXPECT_TRUE(repository.saveCalled);
    EXPECT_TRUE(unitOfWork.commitCalled);
    EXPECT_FALSE(unitOfWork.rollbackCalled);
}

TEST(ReservationApplicationTests, InsufficientStock)
{
    FakeReservationRepository repository;
    repository.storedRequest = createApplicationRequest();

    FakeUnitOfWork unitOfWork;
    FakeStockAvailability stock;
    stock.availableQuantity = 2;

    ReservationService service(repository, unitOfWork, stock);
    auto result = service.processRequest(RequestId("REQ-APP-001"));

    EXPECT_EQ(result.status, ReservationStatus::Rejected);
    EXPECT_FALSE(result.reservation.has_value());
    EXPECT_FALSE(result.rejectionReason.empty());
    EXPECT_TRUE(repository.saveCalled);
    EXPECT_TRUE(unitOfWork.commitCalled);
    EXPECT_FALSE(unitOfWork.rollbackCalled);
}

TEST(ReservationApplicationTests, RequestNotFound)
{
    FakeReservationRepository repository;
    FakeUnitOfWork unitOfWork;
    FakeStockAvailability stock;

    ReservationService service(repository, unitOfWork, stock);

    EXPECT_THROW(
        service.processRequest(RequestId("UNKNOWN")),
        std::runtime_error);
    EXPECT_FALSE(unitOfWork.commitCalled);
    EXPECT_FALSE(unitOfWork.rollbackCalled);
}

TEST(ReservationApplicationTests, StockServiceException)
{
    FakeReservationRepository repository;
    repository.storedRequest = createApplicationRequest();

    FakeUnitOfWork unitOfWork;
    FakeStockAvailability stock;
    stock.fail = true;

    ReservationService service(repository, unitOfWork, stock);

    EXPECT_THROW(
        service.processRequest(RequestId("REQ-APP-001")),
        std::runtime_error);
    EXPECT_TRUE(unitOfWork.rollbackCalled);
    EXPECT_FALSE(unitOfWork.commitCalled);
    EXPECT_FALSE(repository.saveCalled);
}

TEST(ReservationApplicationTests, CommitFailure)
{
    FakeReservationRepository repository;
    repository.storedRequest = createApplicationRequest();

    FakeUnitOfWork unitOfWork;
    unitOfWork.failCommit = true;

    FakeStockAvailability stock;
    stock.availableQuantity = 10;

    ReservationService service(repository, unitOfWork, stock);

    EXPECT_THROW(
        service.processRequest(RequestId("REQ-APP-001")),
        std::runtime_error);
    EXPECT_TRUE(unitOfWork.commitCalled);
    EXPECT_TRUE(unitOfWork.rollbackCalled);
    EXPECT_TRUE(repository.saveCalled);
}

TEST(ReservationApplicationTests, SaveFailure)
{
    FakeReservationRepository repository;
    repository.storedRequest = createApplicationRequest();
    repository.failSave = true;

    FakeUnitOfWork unitOfWork;
    FakeStockAvailability stock;
    stock.availableQuantity = 10;

    ReservationService service(repository, unitOfWork, stock);

    EXPECT_THROW(
        service.processRequest(RequestId("REQ-APP-001")),
        std::runtime_error);
    EXPECT_TRUE(repository.saveCalled);
    EXPECT_TRUE(unitOfWork.rollbackCalled);
    EXPECT_FALSE(unitOfWork.commitCalled);
}
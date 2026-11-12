#include "../src/Model/ReservationRequest.h"
#include "../src/Application/Services/ReservationService.h"
#include <iostream>
#include <stdexcept>
#include <string>
#include <optional>

using namespace ReservationDomain;
using namespace ReservationApplication;
int passedTests = 0;
int failedTests = 0;

void check(const std::string &testName, bool condition)
{
    if (condition)
    {
        std::cout << "[PASSED] " << testName << '\n';
        passedTests++;
    }
    else
    {
        std::cout << "[FAILED] " << testName << '\n';
        failedTests++;
    }
}

template <typename ExceptionType, typename Function>
void checkThrows(
    const std::string &testName,
    Function function)
{
    try
    {
        function();
        check(testName, false);
    }
    catch (const ExceptionType &)
    {
        check(testName, true);
    }
    catch (...)
    {
        check(testName, false);
    }
}

ReservationRequest createRequest(int requestedQuantity = 10)
{
    return ReservationRequest(
        RequestId("REQ-001"),
        ProductId("PROD-001"),
        DepartmentId("DEPT-001"),
        Quantity(requestedQuantity));
}

void testRequestCreation()
{
    ReservationRequest request = createRequest();

    check(
        "New request has Submitted status",
        request.getStatus() == ReservationStatus::Submitted);
}

void testSuccessfulReservation()
{
    ReservationRequest request = createRequest(10);

    std::optional<Reservation> result =
        request.process(AvailableQuantity(15));

    check(
        "Reservation succeeds when stock is sufficient",
        result.has_value());

    check(
        "Successful request has Reserved status",
        request.getStatus() == ReservationStatus::Reserved);

    check(
        "Reservation contains correct request ID",
        result.has_value() &&
            result->requestId.getValue() == "REQ-001");

    check(
        "Reservation contains correct product ID",
        result.has_value() &&
            result->productId.getValue() == "PROD-001");

    check(
        "Reservation contains correct department ID",
        result.has_value() &&
            result->departmentId.getValue() == "DEPT-001");

    check(
        "Reservation contains correct quantity",
        result.has_value() &&
            result->quantity.getValue() == 10);
}

void testReservationWithEqualQuantity()
{
    ReservationRequest request = createRequest(10);

    std::optional<Reservation> result =
        request.process(AvailableQuantity(10));

    check(
        "Reservation succeeds when requested and available quantities are equal",
        result.has_value());

    check(
        "Equal quantity reservation has Reserved status",
        request.getStatus() == ReservationStatus::Reserved);

    check(
        "Equal quantity reservation contains requested quantity",
        result.has_value() &&
            result->quantity.getValue() == 10);
}

void testInsufficientStock()
{
    ReservationRequest request = createRequest(10);

    std::optional<Reservation> result =
        request.process(AvailableQuantity(5));

    check(
        "Reservation is rejected when stock is insufficient",
        !result.has_value());

    check(
        "Request has Rejected status",
        request.getStatus() == ReservationStatus::Rejected);

    check(
        "Rejection reason is provided correctly with quantities",
        request.getRejectionReason() ==
            "Недостатня кількість товару. Запитано: 10, Доступно: 5");
}

void testZeroAvailableStock()
{
    ReservationRequest request = createRequest(1);

    std::optional<Reservation> result =
        request.process(AvailableQuantity(0));

    check(
        "Reservation is rejected when no stock is available",
        !result.has_value());

    check(
        "Request has Rejected status for zero stock",
        request.getStatus() == ReservationStatus::Rejected);

    check(
        "Rejection reason for zero stock is provided correctly",
        request.getRejectionReason() ==
            "Недостатня кількість товару. Запитано: 1, Доступно: 0");
}

void testInvalidRequestedQuantity()
{
    checkThrows<std::invalid_argument>(
        "Zero requested quantity is invalid",
        []()
        {
            Quantity quantity(0);
        });
}

void testNegativeAvailableQuantity()
{
    checkThrows<std::invalid_argument>(
        "Negative available quantity is invalid",
        []()
        {
            AvailableQuantity quantity(-1);
        });
}

void testEmptyRequestId()
{
    checkThrows<std::invalid_argument>(
        "Empty request ID is invalid",
        []()
        {
            RequestId id("");
        });
}

void testRepeatedProcessing()
{
    ReservationRequest request = createRequest(10);

    request.process(AvailableQuantity(15));

    checkThrows<std::logic_error>(
        "Already processed request cannot be processed again",
        [&request]()
        {
            request.process(AvailableQuantity(15));
        });
}

void testEntityIdentity()
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

    check(
        "Requests with different IDs are different entities",
        !(request1.getRequestId() == request2.getRequestId()));
    check(
        "Request identity remains unchanged after processing",
        [&request1]()
        {
            request1.process(AvailableQuantity(15));
            return request1.getRequestId() == RequestId("REQ-001");
        }());
}

void testValueObjectEquality()
{
    Quantity quantity1(10);
    Quantity quantity2(10);
    Quantity quantity3(15);

    check(
        "Quantities with equal values are equal",
        quantity1 == quantity2);

    check(
        "Quantities with different values are not equal",
        !(quantity1 == quantity3));

    RequestId id1("REQ-001");
    RequestId id2("REQ-001");
    RequestId id3("REQ-002");

    check(
        "Request IDs with equal values are equal",
        id1 == id2);

    check(
        "Request IDs with different values are not equal",
        !(id1 == id3));
}

void testStateUnchangedAfterFailedProcessing()
{
    ReservationRequest request = createRequest(10);

    request.process(AvailableQuantity(15));

    ReservationStatus previousStatus = request.getStatus();
    std::string previousReason = request.getRejectionReason();

    checkThrows<std::logic_error>(
        "Repeated processing throws an exception",
        [&request]()
        {
            request.process(AvailableQuantity(15));
        });

    check(
        "Status remains unchanged after failed processing",
        request.getStatus() == previousStatus);

    check(
        "Rejection reason remains unchanged after failed processing",
        request.getRejectionReason() == previousReason);
}

// 4 лабораторна

void testRestoreSubmitted()
{
    ReservationRequest request = ReservationRequest::restore(
        RequestId("REQ-001"),
        ProductId("PROD-001"),
        DepartmentId("DEPT-001"),
        Quantity(10),
        ReservationStatus::Submitted,
        "");

    check(
        "Restored request has Submitted status",
        request.getStatus() == ReservationStatus::Submitted);

    check(
        "Restored Submitted request has empty rejection reason",
        request.getRejectionReason().empty());

    check(
        "Restored request contains correct quantity",
        request.getRequestedQuantity().getValue() == 10);
}

void testRestoreReserved()
{
    ReservationRequest request = ReservationRequest::restore(
        RequestId("REQ-002"),
        ProductId("PROD-001"),
        DepartmentId("DEPT-001"),
        Quantity(10),
        ReservationStatus::Reserved,
        "");

    check(
        "Restored request has Reserved status",
        request.getStatus() == ReservationStatus::Reserved);

    checkThrows<std::logic_error>(
        "Restored Reserved request cannot be processed again",
        [&request]()
        {
            request.process(AvailableQuantity(15));
        });
}

void testRestoreRejected()
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

    check(
        "Restored request has Rejected status",
        request.getStatus() == ReservationStatus::Rejected);

    check(
        "Rejection reason is preserved after restoration",
        request.getRejectionReason() == reason);

    checkThrows<std::logic_error>(
        "Restored Rejected request cannot be processed again",
        [&request]()
        {
            request.process(AvailableQuantity(15));
        });
}

void testRestoreInvalidStates()
{
    checkThrows<std::invalid_argument>(
        "Rejected request without reason cannot be restored",
        []()
        {
            ReservationRequest::restore(
                RequestId("REQ-004"),
                ProductId("PROD-001"),
                DepartmentId("DEPT-001"),
                Quantity(10),
                ReservationStatus::Rejected,
                "");
        });

    checkThrows<std::invalid_argument>(
        "Submitted request with rejection reason cannot be restored",
        []()
        {
            ReservationRequest::restore(
                RequestId("REQ-005"),
                ProductId("PROD-001"),
                DepartmentId("DEPT-001"),
                Quantity(10),
                ReservationStatus::Submitted,
                "Some reason");
        });

    checkThrows<std::invalid_argument>(
        "Reserved request with rejection reason cannot be restored",
        []()
        {
            ReservationRequest::restore(
                RequestId("REQ-006"),
                ProductId("PROD-001"),
                DepartmentId("DEPT-001"),
                Quantity(10),
                ReservationStatus::Reserved,
                "Some reason");
        });

    checkThrows<std::invalid_argument>(
        "Unknown status cannot be restored",
        []()
        {
            ReservationRequest::restore(
                RequestId("REQ-007"),
                ProductId("PROD-001"),
                DepartmentId("DEPT-001"),
                Quantity(10),
                static_cast<ReservationStatus>(999),
                "");
        });
}

// ================= APPLICATION TESTS =================

class FakeReservationRepository : public IReservationRepository
{
public:
  std::optional<ReservationRequest> storedRequest;
  bool saveCalled = false;
  bool failSave = false;
  std::optional<ReservationRequest> findById(
    const RequestId& requestId) override
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

  void save(const ReservationRequest& request) override
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
    const ProductId& productId) override
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

// 1. Успішне резервування
void testApplicationSuccessfulReservation()
{
  FakeReservationRepository repository;
  repository.storedRequest = createApplicationRequest();

  FakeUnitOfWork unitOfWork;

  FakeStockAvailability stock;
  stock.availableQuantity = 10;

  ReservationService service(repository, unitOfWork, stock);

  auto result = service.processRequest(RequestId("REQ-APP-001"));

  check(
    "Application: successful reservation returns Reserved status",
    result.status == ReservationStatus::Reserved);

  check(
    "Application: successful reservation is returned",
    result.reservation.has_value());

  check(
    "Application: request is saved",
    repository.saveCalled);

  check(
    "Application: transaction is committed",
    unitOfWork.commitCalled && !unitOfWork.rollbackCalled);
}

// 2. Недостатня кількість товару
void testApplicationInsufficientStock()
{
  FakeReservationRepository repository;
  repository.storedRequest = createApplicationRequest();

  FakeUnitOfWork unitOfWork;

  FakeStockAvailability stock;
  stock.availableQuantity = 2;

  ReservationService service(repository, unitOfWork, stock);

  auto result = service.processRequest(RequestId("REQ-APP-001"));

  check(
    "Application: insufficient stock returns Rejected status",
    result.status == ReservationStatus::Rejected);

  check(
    "Application: rejected request has no reservation",
    !result.reservation.has_value());

  check(
    "Application: rejection reason is provided",
    !result.rejectionReason.empty());

  check(
    "Application: rejected request is saved and committed",
    repository.saveCalled && unitOfWork.commitCalled);

  check(
    "Application: business rejection does not cause rollback",
    !unitOfWork.rollbackCalled);
}

// 3. Запит не знайдено
void testApplicationRequestNotFound()
{
  FakeReservationRepository repository;

  FakeUnitOfWork unitOfWork;
  FakeStockAvailability stock;

  ReservationService service(repository, unitOfWork, stock);

  checkThrows<std::runtime_error>(
    "Application: missing request throws an exception",
    [&service]()
    {
      service.processRequest(RequestId("UNKNOWN"));
    });

  check(
    "Application: missing request does not commit",
    !unitOfWork.commitCalled);

  check(
    "Application: missing request does not call rollback",
    !unitOfWork.rollbackCalled);
}

// 4. Помилка сервісу перевірки залишків
void testApplicationStockServiceException()
{
  FakeReservationRepository repository;
  repository.storedRequest = createApplicationRequest();

  FakeUnitOfWork unitOfWork;

  FakeStockAvailability stock;
  stock.fail = true;

  ReservationService service(repository, unitOfWork, stock);

  checkThrows<std::runtime_error>(
    "Application: stock service exception is propagated",
    [&service]()
    {
      service.processRequest(RequestId("REQ-APP-001"));
    });

  check(
    "Application: stock service exception causes rollback",
    unitOfWork.rollbackCalled);

  check(
    "Application: stock service exception does not commit",
    !unitOfWork.commitCalled);

  check(
    "Application: request is not saved after stock service exception",
    !repository.saveCalled);
}

// 5. Помилка commit
void testApplicationCommitFailure()
{
  FakeReservationRepository repository;
  repository.storedRequest = createApplicationRequest();

  FakeUnitOfWork unitOfWork;
  unitOfWork.failCommit = true;

  FakeStockAvailability stock;
  stock.availableQuantity = 10;

  ReservationService service(repository, unitOfWork, stock);

  checkThrows<std::runtime_error>(
    "Application: commit exception is propagated",
    [&service]()
    {
      service.processRequest(RequestId("REQ-APP-001"));
    });

  check(
    "Application: commit failure causes rollback",
    unitOfWork.commitCalled && unitOfWork.rollbackCalled);

  check(
    "Application: request was passed to repository before commit",
    repository.saveCalled);
}
// 6. Save failure
void testApplicationSaveFailure()
{
  FakeReservationRepository repository;
  repository.storedRequest = createApplicationRequest();
  repository.failSave = true;

  FakeUnitOfWork unitOfWork;

  FakeStockAvailability stock;
  stock.availableQuantity = 10;

  ReservationService service(repository, unitOfWork, stock);

  checkThrows<std::runtime_error>(
    "Application: save exception is propagated",
    [&service]()
    {
      service.processRequest(RequestId("REQ-APP-001"));
    });

  check(
    "Application: repository save was attempted",
    repository.saveCalled);

  check(
    "Application: save failure causes rollback",
    unitOfWork.rollbackCalled);

  check(
    "Application: save failure does not commit",
    !unitOfWork.commitCalled);
}
int main()
{
    std::cout << "=== RESERVATION DOMAIN TESTS ===\n\n";

    testRequestCreation();
    testSuccessfulReservation();
    testReservationWithEqualQuantity();
    testInsufficientStock();
    testZeroAvailableStock();
    testInvalidRequestedQuantity();
    testNegativeAvailableQuantity();
    testEmptyRequestId();
    testRepeatedProcessing();
    testEntityIdentity();
    testValueObjectEquality();
    testStateUnchangedAfterFailedProcessing();
    // 4 лабораторна
    testRestoreSubmitted();
    testRestoreReserved();
    testRestoreRejected();
    testRestoreInvalidStates();
    // Application tests
    std::cout << "\n=== RESERVATION APPLICATION TESTS ===\n\n";
    testApplicationSuccessfulReservation();
    testApplicationInsufficientStock();
    testApplicationRequestNotFound();
    testApplicationStockServiceException();
    testApplicationCommitFailure();
    testApplicationSaveFailure();
    std::cout << "\n=== TEST SUMMARY ===\n";
    std::cout << "Passed: " << passedTests << '\n';
    std::cout << "Failed: " << failedTests << '\n';

    if (failedTests > 0)
    {
        return 1;
    }

    return 0;
}

#include "../src/Model/ReservationRequest.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <optional>

using namespace ReservationDomain;

int passedTests = 0;
int failedTests = 0;

void check(const std::string& testName, bool condition)
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
    const std::string& testName,
    Function function)
{
    try
    {
        function();
        check(testName, false);
    }
    catch (const ExceptionType&)
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
    std::cout << "\n=== TEST SUMMARY ===\n";
    std::cout << "Passed: " << passedTests << '\n';
    std::cout << "Failed: " << failedTests << '\n';

    if (failedTests > 0)
    {
        return 1;
    }

    return 0;
}

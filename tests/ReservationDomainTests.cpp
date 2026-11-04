#include "../src/Model/ReservationRequest.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <optional>

using namespace ReservationDomain;

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
}

void testReservationWithEqualQuantity()
{
    ReservationRequest request = createRequest(10);

    std::optional<Reservation> result =
        request.process(AvailableQuantity(10));

    check(
        "Reservation succeeds when requested and available quantities are equal",
        result.has_value());
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
        "Rejection reason is provided",
        request.getRejectionReason() ==
            "Insufficient available quantity");
}

void testZeroAvailableStock()
{
    ReservationRequest request = createRequest(1);

    std::optional<Reservation> result =
        request.process(AvailableQuantity(0));

    check(
        "Reservation is rejected when no stock is available",
        !result.has_value());
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

    std::cout << "\n=== TEST SUMMARY ===\n";
    std::cout << "Passed: " << passedTests << '\n';
    std::cout << "Failed: " << failedTests << '\n';

    if (failedTests > 0)
    {
        return 1;
    }

    return 0;
}
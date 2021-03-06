context("Binomial Test")

test_that("Main table results match", {
  options <- jasptools::analysisOptions("BinomialTest")
  options$variables <- "contBinom"
  options$testValue <- 0.6
  options$hypothesis <- "greaterThanTestValue"
  options$confidenceInterval <- TRUE
  options$VovkSellkeMPR <- TRUE
  results <- jasptools::run("BinomialTest", "test.csv", options, view=FALSE, quiet=TRUE)
  table <- results[["results"]][["binomial"]][["data"]]
  expect_equal_tables(table,
		list("contBinom", 0, 58, 100, 0.58, 0.696739870156555, 1, 0.492841460660175,
				 1, "TRUE", "contBinom", 1, 42, 100, 0.42, 0.999903917924738,
				 1, 0.336479745077558, 1, "FALSE")
  )
})

# test_that("Descriptives plots match", {
#   options <- jasptools::analysisOptions("BinomialTest")
#   options$variables <- "contBinom"
#   options$descriptivesPlots <- TRUE
#   options$descriptivesPlotsConfidenceInterval <- 0.90
#   results <- jasptools::run("BinomialTest", "test.csv", options, view=FALSE, quiet=TRUE)
#
#   testPlot <- results[["state"]][["figures"]][[1]]
#   expect_equal_plots(testPlot, "descriptives-1", dir="BinomialTest")
#
#   testPlot <- results[["state"]][["figures"]][[2]]
#   expect_equal_plots(testPlot, "descriptives-2", dir="BinomialTest")
# })

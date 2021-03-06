\name{jaspObject}
\Rdversion{1.1}
\docType{class}
\alias{jaspObject}

\title{Class "jaspObject"}
\description{
  This class is the root of all jaspObjects and defines the methods and properties that are available in all derived classes. 
  It should not be used directly but any method defined here is available in jaspTable, jaspPlot and all other jaspObjects. 
}
\section{Dependencies}{
  \describe{
  The most important functionality that jaspObject provides for all other objects are those connected with dependencies. Usually when the options of an analysis change it doesn't mean that everything needs to be rerun. 
  For instance, suppose we have an analysis where each column in a list of variables generates a single plot. When a column is added to this list we do not need to plot the others again so it would be useful to keep them.
  The same goes for most analyses as they output multiple objects with specific options only pertaining to certain objects. When an option changes the corresponding object should be reset and rebuilt but the others can be kept.\cr\cr

  In the first place it is necessary that the results from previous runs are remembered, which is done automatically in JASP. \link[=jaspResultsClass]{JaspResults} stores itself at the end of the analysis and reloads whenever it is started again.
  When jaspResults reloads the results from the previous run it checks whether any dependencies are set on all it's objects, if there aren't any the object is deleted. This is to ensure that the default behaviour is to rerun everything.\cr\cr
  
  To keep some object under certain circumstances dependencies should be manually specified on the objects.
  This can be done by using 'setOptionMustContainDependency', 'dependOnTheseOptions', 'copyDependenciesFromJaspObject' and to a lesser extent 'setOptionMustBeDependency'.\cr\cr

  To then check if the object should be recreated in the next analysis run we simply check if the entry is \emph{null} or not. \cr
  So if we made a table like so: \code{jaspResults[["aTable"]] <- createJaspTable()} then it will be reset every time we run the analysis.\cr
  If we want to only have the table be recreated whenever "someOption" changes we do: \code{if(is.null(jaspResults[["aTable"]])) jaspResults[["aTable"] <- createJaspTable(dependencies=c("someOption"))}.
  }
}
\section{Methods}{
  \describe{
		\item{\code{print}:}{Prints the object to screen.}

		\item{\code{setOptionMustBeDependency(optionName, optionValue)}:}{Specifies an option and it's required value, if the analysis is restarted and this option is no longer defined (like that) it will automatically destroy the object. Otherwise it will keep it.}
		\item{\code{setOptionMustContainDependency(optionName, optionEntry)}:}{Specifies an option that should define an array and a required value that should be in it, if the analysis is restarted and this option is no longer defined or no longer contains the specified value it will automatically destroy the object. Otherwise it will keep it.}
		\item{\code{dependOnTheseOptions(charVec)}:}{Will make the object depend on the current values of the options specified in the charactervector.}
		\item{\code{copyDependenciesFromJaspObject(jaspObj)}:}{Will make the object depend on whatever the other jaspObject depends.}
		
		\item{\code{addMessage}:}{Adds a message to this object, it is ignored for now.}
		
	 }
}	
\section{Fields}{
  \describe{
    \item{\code{title}:}{The title of this object.}

	\item{\code{warning}:}{A field to set a warning, this is ignored for now.}
  }
}
\examples{

plotA <- createJaspPlot(plot=SomeFunc())

# Make sure that the plot is reset whenever the specified 'variable' is removed from the option "variables":
plotA$setOptionMustContainDependency("variables", variable) 

# The plot will automatically removed from the results when the option "drawPlotA" changes to FALSE:
plotA$setOptionMustBeDependency("drawPlotA", TRUE)

# An easier method that makes the plot depend on the current values of the options "drawPlotA", "makeAPrettyPlot" and "color" and will be reset whenever either changes:
plotA$dependOnTheseOptions("drawPlotA", "makeAPrettyPlot", "color") 
# This is easier to use then setOptionMustBeDependency and is preferred.

# A header that depends on the same things that plotA depends on:
headerA <- createJaspHtml(text="A Nice Plot", elementType="h2")
headerA$copyDependenciesFromJaspObject(plotA)
}


\keyword{classes}

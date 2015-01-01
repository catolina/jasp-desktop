
#include "rbridge.h"

#include <boost/foreach.hpp>

#include "../JASP-Common/base64.h"
#include "../JASP-Common/lib_json/json.h"
#include "../JASP-Common/sharedmemory.h"

RInside *rbridge_rinside;
DataSet *rbridge_dataSet;

using namespace std;

RCallback rbridge_run_callback;

Rcpp::DataFrame rbridge_readDataSetSEXP(SEXP columns, SEXP columnsAsNumeric, SEXP columnsAsOrdinal, SEXP columnsAsNominal, SEXP allColumns);
Rcpp::DataFrame rbridge_readDataSetHeaderSEXP(SEXP columns, SEXP columnsAsNumeric, SEXP columnsAsOrdinal, SEXP columnsAsNominal, SEXP allColumns);
std::map<std::string, Column::ColumnType> rbridge_marshallSEXPs(SEXP columns, SEXP columnsAsNumeric, SEXP columnsAsOrdinal, SEXP columnsAsNominal, SEXP allColumns);
SEXP rbridge_callbackSEXP(SEXP results);

int rbridge_callback(SEXP results);
Rcpp::DataFrame rbridge_readDataSet(const std::map<std::string, Column::ColumnType> &columns);
Rcpp::DataFrame rbridge_readDataSetHeader(const std::map<std::string, Column::ColumnType> &columns);

void rbridge_makeFactor(Rcpp::IntegerVector &v, const std::vector<std::string> &levels, bool ordinal = false);
void rbridge_makeFactor(Rcpp::IntegerVector &v, const Labels &levels, bool ordinal = false);


void rbridge_init()
{
	rbridge_dataSet = NULL;
	rbridge_run_callback = NULL;

	rbridge_rinside = new RInside();

	RInside &rInside = rbridge_rinside->instance();

	rInside[".readDatasetToEndNative"] = Rcpp::InternalFunction(&rbridge_readDataSetSEXP);
	rInside[".readDataSetHeaderNative"] = Rcpp::InternalFunction(&rbridge_readDataSetHeaderSEXP);
	rInside[".callbackNative"] = Rcpp::InternalFunction(&rbridge_callbackSEXP);
	rInside[".baseCitation"] = "Love, J., Selker, R., Verhagen, J., Smira, M., Wild, A., Marsman, M., Gronau, Q., Morey, R., Rouder, J. & Wagenmakers, E. J. (2014). JASP (Version 0.5)[Computer software].";

	rInside["jasp.analyses"] = Rcpp::List();
	rInside.parseEvalQNT("suppressPackageStartupMessages(library(\"RJSONIO\"))");
	rInside.parseEvalQNT("suppressPackageStartupMessages(library(\"JASP\"))");
	rInside.parseEvalQNT("suppressPackageStartupMessages(library(\"methods\"))");
}

void rbridge_setDataSet(DataSet *dataSet)
{
	rbridge_dataSet = dataSet;
}

string rbridge_run(const string &name, const string &options, const string &perform, RCallback callback)
{
	SEXP results;

	rbridge_run_callback = callback;

	RInside &rInside = rbridge_rinside->instance();

	rInside["name"] = name;
	rInside["options.as.json.string"] = options;
	rInside["perform"] = perform;
	rInside.parseEval("run(name=name, options.as.json.string=options.as.json.string, perform)", results);

	rbridge_run_callback = NULL;

	return Rcpp::as<string>(results);
}

Rcpp::DataFrame rbridge_readDataSet(const std::map<std::string, Column::ColumnType> &columns)
{
	if (rbridge_dataSet == NULL)
	{
		std::cout << "rbridge dataset not set!\n";
		std::cout.flush();
	}

	Rcpp::List list(columns.size());
	Rcpp::CharacterVector columnNames;

	int colNo = 0;

	typedef pair<const string, Column::ColumnType> ColumnInfo;

	BOOST_FOREACH(const ColumnInfo &columnInfo, columns)
	{
		(void)columns;
		string dot = ".";

		string columnName = columnInfo.first;

		string base64 = Base64::encode(dot, columnName);
		columnNames.push_back(base64);

		Column &column = rbridge_dataSet->columns().get(columnName);
		Column::ColumnType columnType = column.columnType();

		Column::ColumnType requestedType = columnInfo.second;
		if (requestedType == Column::ColumnTypeUnknown)
			requestedType = columnType;

		int rowCount = column.rowCount();
		int rowNo = 0;

		if (requestedType == Column::ColumnTypeScale)
		{
			if (columnType == Column::ColumnTypeScale)
			{
				Rcpp::NumericVector v(rowCount);

				BOOST_FOREACH(double value, column.AsDoubles)
				{
					(void)column;
					v[rowNo++] = value;
				}

				list[colNo++] = v;
			}
			else if (columnType == Column::ColumnTypeOrdinal || columnType == Column::ColumnTypeNominal)
			{
				Rcpp::IntegerVector v(rowCount);

				BOOST_FOREACH(int value, column.AsInts)
				{
					(void)column;
					v[rowNo++] = column.actualFromRaw(value);
				}

				list[colNo++] = v;
			}
			else // columnType == Column::ColumnTypeNominalText
			{
				Rcpp::IntegerVector v(rowCount);

				BOOST_FOREACH(int value, column.AsInts)
				{
					(void)column;
					if (value == INT_MIN)
						v[rowNo++] = INT_MIN;
					else
						v[rowNo++] = value + 1;
				}

				rbridge_makeFactor(v, column.labels());

				list[colNo++] = v;
			}
		}
		else // if (requestedType != Column::ColumnTypeScale)
		{
			bool ordinal = (requestedType == Column::ColumnTypeOrdinal);

			Rcpp::IntegerVector v(rowCount);

			if (columnType != Column::ColumnTypeScale)
			{
				BOOST_FOREACH(int value, column.AsInts)
				{
					(void)column;
					if (value == INT_MIN)
						v[rowNo++] = INT_MIN;
					else
						v[rowNo++] = value + 1;
				}

				rbridge_makeFactor(v, column.labels(), ordinal);
			}
			else
			{
				// scale to nominal or ordinal (doesn't really make sense, but we have to do something)

				set<int> uniqueValues;

				BOOST_FOREACH(double value, column.AsDoubles)
				{
					(void)column;
					int intValue = (int)(value * 1000);
					uniqueValues.insert(intValue);
				}

				int index = 0;
				map<int, int> valueToIndex;
				vector<string> labels;

				BOOST_FOREACH(int value, uniqueValues)
				{
					(void)uniqueValues;
					valueToIndex[value] = index;

					stringstream ss;
					ss << ((double)value / 1000);
					labels.push_back(ss.str());

					index++;
				}

				BOOST_FOREACH(double value, column.AsDoubles)
				{
					(void)column;

					if (isnan(value))
						v[rowNo++] = INT_MIN;
					else
					{
						v[rowNo++] = valueToIndex[(int)(value * 1000)] + 1;
					}
				}

				rbridge_makeFactor(v, labels, ordinal);
			}

			list[colNo++] = v;
		}
	}

	list.attr("names") = columnNames;

	Rcpp::DataFrame dataFrame = Rcpp::DataFrame(list);

	return dataFrame;
}

Rcpp::DataFrame rbridge_readDataSetHeader(const std::map<string, Column::ColumnType> &columns)
{
	if (rbridge_dataSet == NULL)
	{
		std::cout << "rbridge dataset not set!\n";
		std::cout.flush();
	}

	Rcpp::List list(columns.size());
	Rcpp::CharacterVector columnNames;

	int colNo = 0;

	typedef pair<const string, Column::ColumnType> ColumnInfo;

	BOOST_FOREACH(const ColumnInfo &columnInfo, columns)
	{
		(void)columns;
		string dot = ".";

		string columnName = columnInfo.first;

		string base64 = Base64::encode(dot, columnName);
		columnNames.push_back(base64);

		Columns &columns = rbridge_dataSet->columns();
		Column &column = columns.get(columnName);
		Column::ColumnType columnType = column.columnType();

		Column::ColumnType requestedType = columnInfo.second;
		if (requestedType == Column::ColumnTypeUnknown)
			requestedType = columnType;

		if (requestedType == Column::ColumnTypeScale)
		{
			if (columnType == Column::ColumnTypeScale)
			{
				list[colNo++] = Rcpp::NumericVector(0);
			}
			else if (columnType == Column::ColumnTypeOrdinal || columnType == Column::ColumnTypeNominal)
			{
				list[colNo++] = Rcpp::IntegerVector(0);
			}
			else
			{
				Rcpp::IntegerVector v(0);
				rbridge_makeFactor(v, column.labels());
				list[colNo++] = v;
			}
		}
		else
		{
			bool ordinal = (requestedType == Column::ColumnTypeOrdinal);

			Rcpp::IntegerVector v(0);
			rbridge_makeFactor(v, column.labels(), ordinal);

			list[colNo++] = v;
		}
	}

	list.attr("names") = columnNames;

	Rcpp::DataFrame dataFrame = Rcpp::DataFrame(list);

	return dataFrame;
}

void rbridge_makeFactor(Rcpp::IntegerVector &v, const Labels &levels, bool ordinal)
{
	Rcpp::CharacterVector labels;

	if (levels.size() == 0)
	{
		labels.push_back(".");
	}
	else
	{
		for (int i = 0; i < levels.size(); i++)
			labels.push_back(levels.at(i).text());
	}

	v.attr("levels") = labels;

	vector<string> cla55;
	if (ordinal)
		cla55.push_back("ordered");
	cla55.push_back("factor");

	v.attr("class") = cla55;
}

void rbridge_makeFactor(Rcpp::IntegerVector &v, const std::vector<string> &levels, bool ordinal)
{
	v.attr("levels") = levels;

	vector<string> cla55;
	if (ordinal)
		cla55.push_back("ordered");
	cla55.push_back("factor");

	v.attr("class") = cla55;
}


int rbridge_callback(SEXP results)
{
	if (rbridge_run_callback != NULL)
	{
		if (Rf_isNull(results))
		{
			return rbridge_run_callback("null");
		}
		else
		{
			return rbridge_run_callback(Rcpp::as<string>(results));
		}
	}
	else
	{
		return 0;
	}
}

std::map<string, Column::ColumnType> rbridge_marshallSEXPs(SEXP columns, SEXP columnsAsNumeric, SEXP columnsAsOrdinal, SEXP columnsAsNominal, SEXP allColumns)
{
	map<string, Column::ColumnType> columnsRequested;

	if (Rf_isLogical(allColumns) && Rcpp::as<bool>(allColumns))
	{
		BOOST_FOREACH(const Column &column, rbridge_dataSet->columns())
			columnsRequested[column.name()] = Column::ColumnTypeUnknown;
	}

	if (Rf_isString(columns))
	{
		vector<string> temp = Rcpp::as<vector<string> >(columns);
		for (int i = 0; i < temp.size(); i++)
			columnsRequested[temp.at(i)] = Column::ColumnTypeUnknown;
	}

	if (Rf_isString(columnsAsNumeric))
	{
		vector<string> temp = Rcpp::as<vector<string> >(columnsAsNumeric);
		for (int i = 0; i < temp.size(); i++)
			columnsRequested[temp.at(i)] = Column::ColumnTypeScale;
	}

	if (Rf_isString(columnsAsOrdinal))
	{
		vector<string> temp = Rcpp::as<vector<string> >(columnsAsOrdinal);
		for (int i = 0; i < temp.size(); i++)
			columnsRequested[temp.at(i)] = Column::ColumnTypeOrdinal;
	}

	if (Rf_isString(columnsAsNominal))
	{
		vector<string> temp = Rcpp::as<vector<string> >(columnsAsNominal);
		for (int i = 0; i < temp.size(); i++)
			columnsRequested[temp.at(i)] = Column::ColumnTypeNominal;
	}

	return columnsRequested;
}

SEXP rbridge_callbackSEXP(SEXP results)
{
	Rcpp::NumericVector control(1);
	control[0] = rbridge_callback(results);
	return control;
}

Rcpp::DataFrame rbridge_readDataSetSEXP(SEXP columns, SEXP columnsAsNumeric, SEXP columnsAsOrdinal, SEXP columnsAsNominal, SEXP allColumns)
{
	map<string, Column::ColumnType> columnsRequested = rbridge_marshallSEXPs(columns, columnsAsNumeric, columnsAsOrdinal, columnsAsNominal, allColumns);
	return rbridge_readDataSet(columnsRequested);
}

Rcpp::DataFrame rbridge_readDataSetHeaderSEXP(SEXP columns, SEXP columnsAsNumeric, SEXP columnsAsOrdinal, SEXP columnsAsNominal, SEXP allColumns)
{
	map<string, Column::ColumnType> columnsRequested = rbridge_marshallSEXPs(columns, columnsAsNumeric, columnsAsOrdinal, columnsAsNominal, allColumns);
	return rbridge_readDataSetHeader(columnsRequested);
}


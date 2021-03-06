/*
 * Copyright (C) 2013 Emeric Poupon
 *
 * This file is part of LMS.
 *
 * LMS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LMS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LMS.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <vector>

#include <Wt/WFormModel>
#include <Wt/WLineEdit>
#include <Wt/WPushButton>
#include <Wt/Auth/Identity>

#include "logger/Logger.hpp"

#include "common/Validators.hpp"
#include "LmsApplication.hpp"

#include "SettingsFirstConnectionFormView.hpp"

namespace UserInterface {
namespace Settings {

class FirstConnectionFormModel : public Wt::WFormModel
{
	public:

		// Associate each field with a unique string literal.
		static const Field NameField;
		static const Field EmailField;
		static const Field PasswordField;
		static const Field PasswordConfirmField;

		FirstConnectionFormModel(Wt::WObject *parent = 0)
			: Wt::WFormModel(parent)
		{
			addField(NameField);
			addField(EmailField);
			addField(PasswordField);
			addField(PasswordConfirmField);

			setValidator(NameField, createNameValidator());
			setValidator(EmailField, createEmailValidator());

		}

		bool saveData(Wt::WString& error)
		{
			// DBO transaction active here
			try {
				Wt::Dbo::Transaction transaction(DboSession());

				// Check if a user already exist
				// If it's the case, just do nothing
				if (!Database::User::getAll(DboSession()).empty())
				{
					LMS_LOG(UI, ERROR) << "Admin user already created";
					error = Wt::WString("Admin user already created!");
					return false;
				}

				// Create user
				Wt::Auth::User authUser = DbHandler().getUserDatabase().registerNew();
				Database::User::pointer user = DbHandler().getUser(authUser);

				// Account
				authUser.setIdentity(Wt::Auth::Identity::LoginName, valueText(NameField));
				authUser.setEmail(valueText(EmailField).toUTF8());
				Database::Handler::getPasswordService().updatePassword(authUser, valueText(PasswordField));

				user.modify()->setAdmin( true );

			}
			catch(Wt::Dbo::Exception& exception)
			{
				LMS_LOG(UI, ERROR) << "Dbo exception: " << exception.what();
				error = Wt::WString(exception.what());
				return false;
			}

			return true;
		}

		bool validateField(Field field)
		{
			// DBO transaction active here

			Wt::WString error;

			if (field == PasswordField)
			{
				// Password is mandatory if we create the user
				if (!valueText(PasswordField).empty())
				{
					// Evaluate the strength of the password
					Wt::Auth::AbstractPasswordService::StrengthValidatorResult res
						= Database::Handler::getPasswordService().strengthValidator()->evaluateStrength(valueText(PasswordField),
								valueText(NameField),
								valueText(EmailField).toUTF8());

					if (!res.isValid())
						error = res.message();
				}
				else
					return Wt::WFormModel::validateField(field);
			}
			else if (field == PasswordConfirmField)
			{
				if (validation(PasswordField).state() == Wt::WValidator::Valid)
				{
					if (valueText(PasswordField) != valueText(PasswordConfirmField))
						error = Wt::WString::tr("Wt.Auth.passwords-dont-match");
				}
			}
			else
			{
				// Apply validators
				return Wt::WFormModel::validateField(field);
			}

			setValidation(field, Wt::WValidator::Result( error.empty() ? Wt::WValidator::Valid : Wt::WValidator::Invalid, error));

			return validation(field).state() == Wt::WValidator::Valid;
		}


	private:

};

const Wt::WFormModel::Field FirstConnectionFormModel::NameField = "name";
const Wt::WFormModel::Field FirstConnectionFormModel::EmailField = "email";
const Wt::WFormModel::Field FirstConnectionFormModel::PasswordField = "password";
const Wt::WFormModel::Field FirstConnectionFormModel::PasswordConfirmField = "password-confirm";

FirstConnectionFormView::FirstConnectionFormView(Wt::WContainerWidget *parent)
: Wt::WTemplateFormView(parent)
{

	_model = new FirstConnectionFormModel(this);

	setTemplateText(tr("firstConnectionForm-template"));
	addFunction("id", &WTemplate::Functions::id);
	addFunction("block", &WTemplate::Functions::id);

	_applyInfo = new Wt::WText();
	_applyInfo->setInline(false);
	_applyInfo->hide();
	bindWidget("apply-info", _applyInfo);

	// Name
	Wt::WLineEdit* accountEdit = new Wt::WLineEdit();
	setFormWidget(FirstConnectionFormModel::NameField, accountEdit);
	accountEdit->changed().connect(_applyInfo, &Wt::WWidget::hide);

	// Email
	Wt::WLineEdit* emailEdit = new Wt::WLineEdit();
	setFormWidget(FirstConnectionFormModel::EmailField, emailEdit);
	emailEdit->changed().connect(_applyInfo, &Wt::WWidget::hide);

	// Password
	Wt::WLineEdit* passwordEdit = new Wt::WLineEdit();
	setFormWidget(FirstConnectionFormModel::PasswordField, passwordEdit );
	passwordEdit->setEchoMode(Wt::WLineEdit::Password);
	passwordEdit->changed().connect(_applyInfo, &Wt::WWidget::hide);

	// Password confirmation
	Wt::WLineEdit* passwordConfirmEdit = new Wt::WLineEdit();
	setFormWidget(FirstConnectionFormModel::PasswordConfirmField, passwordConfirmEdit);
	passwordConfirmEdit->setEchoMode(Wt::WLineEdit::Password);
	passwordConfirmEdit->changed().connect(_applyInfo, &Wt::WWidget::hide);

	// Title & Buttons
	bindString("title", "Create Admin account");

	_saveButton = new Wt::WPushButton("Create");
	_saveButton->setStyleClass("btn-primary");
	bindWidget("save-button", _saveButton);
	_saveButton->clicked().connect(this, &FirstConnectionFormView::processSave);

	updateView(_model);

}

void
FirstConnectionFormView::processSave()
{
	updateModel(_model);

	_applyInfo->show();
	if (_model->validate())
	{
		Wt::WString error;
		// commit model into DB
		if (_model->saveData(error) )
		{
			_applyInfo->setText( Wt::WString::fromUTF8("New parameters successfully applied! Please refresh this page in order to login"));
			_applyInfo->setStyleClass("alert alert-success");
			_saveButton->hide();
		}
		else
		{
			_applyInfo->setText( Wt::WString::fromUTF8("Cannot apply new parameters: ") + error);
			_applyInfo->setStyleClass("alert alert-danger");
		}
	}
	else
	{
		_applyInfo->setText( Wt::WString::fromUTF8("Cannot apply new parameters!"));
		_applyInfo->setStyleClass("alert alert-danger");
	}
	updateView(_model);
}

} // namespace Settings
} // namespace UserInterface

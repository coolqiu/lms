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
#include <Wt/WCheckBox>
#include <Wt/WComboBox>
#include <Wt/WPushButton>
#include <Wt/Auth/Identity>

#include "logger/Logger.hpp"

#include "common/Validators.hpp"
#include "LmsApplication.hpp"

#include "SettingsAccountFormView.hpp"

namespace UserInterface {
namespace Settings {

class AccountFormModel : public Wt::WFormModel
{
	public:

		// Associate each field with a unique string literal.
		static const Field NameField;
		static const Field EmailField;
		static const Field PasswordField;
		static const Field PasswordConfirmField;

		AccountFormModel(std::string userId, Wt::WObject *parent = 0)
			: Wt::WFormModel(parent),
			_userId(userId)
		{
			addField(NameField);
			addField(EmailField);
			addField(PasswordField);
			addField(PasswordConfirmField);

			setValidator(NameField, createNameValidator());
			setValidator(EmailField, createEmailValidator());

			// populate the model with initial data
			loadData();
		}

		void loadData()
		{
			Wt::Dbo::Transaction transaction(DboSession());

			Wt::Auth::User authUser = DbHandler().getUserDatabase().findWithId( _userId );
			Database::User::pointer user = DbHandler().getUser(authUser);

			if (user && authUser.isValid())
			{
				setValue(NameField, authUser.identity(Wt::Auth::Identity::LoginName));
				if (!authUser.email().empty())
					setValue(EmailField, authUser.email());
				else
					setValue(EmailField, authUser.unverifiedEmail());
			}
			setValue(PasswordField, Wt::WString());
			setValue(PasswordConfirmField, Wt::WString());
		}

		bool saveData(Wt::WString& error)
		{
			// DBO transaction active here
			try {
				Wt::Dbo::Transaction transaction(DboSession());

				// Update user
				Wt::Auth::User authUser = DbHandler().getUserDatabase().findWithId(_userId);
				Database::User::pointer user = DbHandler().getUser( authUser );

				// user may have been deleted by someone else
				if (!authUser.isValid()) {
					error = Wt::WString("User identity does not exist");
					return false;
				}
				else if(!user)
				{
					error = Wt::WString("User not found");
					return false;
				}

				// Account
				authUser.setIdentity(Wt::Auth::Identity::LoginName, valueText(NameField));
				authUser.setEmail(valueText(EmailField).toUTF8());

				// Password
				if (!valueText(PasswordField).empty())
					Database::Handler::getPasswordService().updatePassword(authUser, valueText(PasswordField));

				setValue(PasswordField, Wt::WString());
				setValue(PasswordConfirmField, Wt::WString());

			}
			catch(Wt::Dbo::Exception& exception)
			{
				LMS_LOG(UI, ERROR) << "Dbo exception: " << exception.what();
				return false;
			}

			return true;
		}

		bool validateField(Field field)
		{
			// DBO transaction active here

			Wt::WString error;

			if (field == NameField)
			{
				Wt::Dbo::Transaction transaction(DboSession());
				// Must be unique since used as LoginIdentity
				Wt::Auth::User user = DbHandler().getUserDatabase().findWithIdentity(Wt::Auth::Identity::LoginName, valueText(field));
				if (user.isValid() && user.id() != _userId)
					error = "Already exists";
				else
					return Wt::WFormModel::validateField(field);
			}
			else if (field == PasswordField)
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

		std::string		_userId;
};

const Wt::WFormModel::Field AccountFormModel::NameField = "name";
const Wt::WFormModel::Field AccountFormModel::EmailField = "email";
const Wt::WFormModel::Field AccountFormModel::PasswordField = "password";
const Wt::WFormModel::Field AccountFormModel::PasswordConfirmField = "password-confirm";

AccountFormView::AccountFormView(std::string userId, Wt::WContainerWidget *parent)
: Wt::WTemplateFormView(parent)
{

	_model = new AccountFormModel(userId, this);

	setTemplateText(tr("userAccountForm-template"));
	addFunction("id", &WTemplate::Functions::id);
	addFunction("block", &WTemplate::Functions::id);

	_applyInfo = new Wt::WText();
	_applyInfo->setInline(false);
	_applyInfo->hide();
	bindWidget("apply-info", _applyInfo);

	// Name
	Wt::WLineEdit* accountEdit = new Wt::WLineEdit();
	setFormWidget(AccountFormModel::NameField, accountEdit);
	accountEdit->changed().connect(_applyInfo, &Wt::WWidget::hide);

	// Email
	Wt::WLineEdit* emailEdit = new Wt::WLineEdit();
	setFormWidget(AccountFormModel::EmailField, emailEdit);
	emailEdit->changed().connect(_applyInfo, &Wt::WWidget::hide);

	// Password
	Wt::WLineEdit* passwordEdit = new Wt::WLineEdit();
	setFormWidget(AccountFormModel::PasswordField, passwordEdit );
	passwordEdit->setEchoMode(Wt::WLineEdit::Password);
	passwordEdit->changed().connect(_applyInfo, &Wt::WWidget::hide);

	// Password confirmation
	Wt::WLineEdit* passwordConfirmEdit = new Wt::WLineEdit();
	setFormWidget(AccountFormModel::PasswordConfirmField, passwordConfirmEdit);
	passwordConfirmEdit->setEchoMode(Wt::WLineEdit::Password);
	passwordConfirmEdit->changed().connect(_applyInfo, &Wt::WWidget::hide);

	// Title & Buttons
	bindString("title", "Account settings");

	Wt::WPushButton *saveButton = new Wt::WPushButton("Apply");
	saveButton->setStyleClass("btn-primary");
	bindWidget("save-button", saveButton);
	saveButton->clicked().connect(this, &AccountFormView::processSave);

	Wt::WPushButton *cancelButton = new Wt::WPushButton("Discard");
	bindWidget("cancel-button", cancelButton);
	cancelButton->clicked().connect(this, &AccountFormView::processCancel);

	updateView(_model);

}

	void
AccountFormView::processCancel()
{
	_applyInfo->show();

	_applyInfo->setText( Wt::WString::fromUTF8("Parameters reverted!"));
	_applyInfo->setStyleClass("alert alert-info");
	_model->loadData();

	_model->validate();
	updateView(_model);
}

void
AccountFormView::processSave()
{
	updateModel(_model);

	_applyInfo->show();
	if (_model->validate())
	{
		Wt::WString error;
		// commit model into DB
		if (_model->saveData(error) )
		{
			_applyInfo->setText( Wt::WString::fromUTF8("New parameters successfully applied!"));
			_applyInfo->setStyleClass("alert alert-success");
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
